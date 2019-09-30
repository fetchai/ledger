#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "math/clustering/knn.hpp"
#include "ml/distributed_learning/distributed_learning_client.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "word2vec_training_params.hpp"

namespace fetch {
namespace ml {
namespace distributed_learning {

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  if (t.fail())
  {
    throw std::runtime_error("Cannot open file " + path);
  }

  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

template <class TensorType>
class Word2VecClient : public TrainingClient<TensorType>
{
  using DataType         = typename TensorType::Type;
  using SizeType         = typename TensorType::SizeType;
  using VectorTensorType = std::vector<TensorType>;

public:
  Word2VecClient(std::string const &id, W2VTrainingParams<DataType> const &tp,
                 std::shared_ptr<std::mutex> console_mutex_ptr);

  void PrepareModel();

  void PrepareDataLoader();

  void PrepareOptimiser();

  void Test() override;

private:
  W2VTrainingParams<DataType>                                       tp_;
  std::string                                                       skipgram_;
  std::shared_ptr<fetch::ml::dataloaders::GraphW2VLoader<DataType>> w2v_data_loader_ptr_;

  void PrintWordAnalogy(TensorType const &embeddings, std::string const &word1,
                        std::string const &word2, std::string const &word3, SizeType k);

  void PrintKNN(TensorType const &embeddings, std::string const &word0, SizeType k);

  void TestEmbeddings(std::string const &word0, std::string const &word1, std::string const &word2,
                      std::string const &word3, SizeType K);
};

template <class TensorType>
Word2VecClient<TensorType>::Word2VecClient(std::string const &                id,
                                           W2VTrainingParams<DataType> const &tp,
                                           std::shared_ptr<std::mutex>        console_mutex_ptr)
  : TrainingClient<TensorType>(id, tp, console_mutex_ptr)
  , tp_(tp)
{
  PrepareDataLoader();
  PrepareModel();

  DataType est_samples = w2v_data_loader_ptr_->EstimatedSampleNumber();
  // calc the compatiable linear lr decay
  tp_.learning_rate_param.linear_decay_rate = DataType{1} / est_samples;
  // this decay rate gurantees that the lr is reduced to zero by the
  // end of an epoch (despite capping by ending learning rate)
  std::cout << "id: " << id << ", dataloader_.EstimatedSampleNumber(): " << est_samples
            << std::endl;

  PrepareOptimiser();
}

template <class TensorType>
void Word2VecClient<TensorType>::PrepareModel()
{
  this->g_ptr_ = std::make_shared<fetch::ml::Graph<TensorType>>();

  std::string input_name =
      this->g_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string context_name =
      this->g_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Context", {});
  this->label_name_ =
      this->g_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});
  skipgram_ = this->g_ptr_->template AddNode<fetch::ml::layers::SkipGram<TensorType>>(
      "SkipGram", {input_name, context_name}, SizeType(1), SizeType(1), tp_.embedding_size,
      w2v_data_loader_ptr_->vocab_size());

  this->error_name_ = this->g_ptr_->template AddNode<fetch::ml::ops::CrossEntropyLoss<TensorType>>(
      "Error", {skipgram_, this->label_name_});

  this->inputs_names_ = {input_name, context_name};
}

template <class TensorType>
void Word2VecClient<TensorType>::PrepareDataLoader()
{
  w2v_data_loader_ptr_ = std::make_shared<fetch::ml::dataloaders::GraphW2VLoader<DataType>>(
      tp_.window_size, tp_.negative_sample_size, tp_.freq_thresh, tp_.max_word_count);
  w2v_data_loader_ptr_->LoadVocab(tp_.vocab_file);
  w2v_data_loader_ptr_->BuildData({tp_.data}, tp_.min_count);

  this->dataloader_ptr_ = w2v_data_loader_ptr_;
}

template <class TensorType>
void Word2VecClient<TensorType>::PrepareOptimiser()
{
  // Initialise Optimiser
  this->opti_ptr_ = std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
      this->g_ptr_, this->inputs_names_, this->label_name_, this->error_name_,
      tp_.learning_rate_param);
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void Word2VecClient<TensorType>::Test()
{
  // TODO(issue 1595): Implement loss mechanism

  if (this->batch_counter_ % tp_.test_frequency == 1)
  {
    TestEmbeddings(tp_.word0, tp_.word1, tp_.word2, tp_.word3, tp_.k);
  }
}

template <class TensorType>
void Word2VecClient<TensorType>::PrintWordAnalogy(TensorType const & embeddings,
                                                  std::string const &word1,
                                                  std::string const &word2,
                                                  std::string const &word3, SizeType k)
{
  if (!w2v_data_loader_ptr_->WordKnown(word1) || !w2v_data_loader_ptr_->WordKnown(word2) ||
      !w2v_data_loader_ptr_->WordKnown(word3))
  {
    throw std::runtime_error("WARNING! not all to-be-tested words are in vocabulary");
  }
  else
  {
    std::cout << "Find word that is to " << word3 << " what " << word2 << " is to " << word1
              << std::endl;
  }

  // get id for words
  SizeType word1_idx = w2v_data_loader_ptr_->IndexFromWord(word1);
  SizeType word2_idx = w2v_data_loader_ptr_->IndexFromWord(word2);
  SizeType word3_idx = w2v_data_loader_ptr_->IndexFromWord(word3);

  // get word vectors for words
  TensorType word1_vec = embeddings.Slice(word1_idx, 1).Copy();
  TensorType word2_vec = embeddings.Slice(word2_idx, 1).Copy();
  TensorType word3_vec = embeddings.Slice(word3_idx, 1).Copy();

  word1_vec /= fetch::math::L2Norm(word1_vec);
  word2_vec /= fetch::math::L2Norm(word2_vec);
  word3_vec /= fetch::math::L2Norm(word3_vec);

  TensorType word4_vec = word2_vec - word1_vec + word3_vec;

  std::vector<std::pair<typename TensorType::SizeType, typename TensorType::Type>> output =
      fetch::math::clustering::KNNCosine(embeddings, word4_vec, k);

  for (std::size_t l = 0; l < output.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << output.at(l).second << ": "
              << w2v_data_loader_ptr_->WordFromIndex(output.at(l).first) << std::endl;
  }
}

template <class TensorType>
void Word2VecClient<TensorType>::PrintKNN(TensorType const &embeddings, std::string const &word0,
                                          SizeType k)
{
  if (w2v_data_loader_ptr_->IndexFromWord(word0) == fetch::math::numeric_max<SizeType>())
  {
    throw std::runtime_error("WARNING! could not find [" + word0 + "] in vocabulary");
  }

  SizeType   idx        = w2v_data_loader_ptr_->IndexFromWord(word0);
  TensorType one_vector = embeddings.Slice(idx, 1).Copy();
  std::vector<std::pair<typename TensorType::SizeType, typename TensorType::Type>> output =
      fetch::math::clustering::KNNCosine(embeddings, one_vector, k);

  for (std::size_t l = 0; l < output.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << output.at(l).second << ": "
              << w2v_data_loader_ptr_->WordFromIndex(output.at(l).first) << std::endl;
  }
}

template <class TensorType>
void Word2VecClient<TensorType>::TestEmbeddings(std::string const &word0, std::string const &word1,
                                                std::string const &word2, std::string const &word3,
                                                SizeType K)
{
  // Lock model
  FETCH_LOCK(this->model_mutex_);

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<TensorType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<TensorType>>(
          (this->g_ptr_->GetNode(skipgram_))->GetOp());

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<TensorType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  {
    // Lock console
    FETCH_LOCK(*this->console_mutex_ptr_);

    std::cout << std::endl;
    std::cout << "Client " << this->id_ << ", batches done = " << this->batch_counter_ << std::endl;
    PrintKNN(embeddings->GetWeights(), word0, K);
    std::cout << std::endl;
    PrintWordAnalogy(embeddings->GetWeights(), word1, word2, word3, K);
  }
}

}  // namespace distributed_learning
}  // namespace ml
}  // namespace fetch
