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

#include "ml/distributed_learning/distributed_learning_client.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "word2vec_training_params.hpp"

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
  Word2VecClient(std::string const &id, TrainingParams<TensorType> const &tp,
                 std::string const &vocab_file, SizeType batch_size, SizeType number_of_peers);
  void PrepareModel() override;
  void PrepareDataLoader() override;
  void PrepareOptimiser() override;
  void Test(DataType &test_loss) override;

private:
  TrainingParams<TensorType>                                        tp_;
  std::string                                                       skipgram_;
  std::string                                                       vocab_file_;
  std::shared_ptr<fetch::ml::dataloaders::GraphW2VLoader<DataType>> w2v_data_loader_ptr_;

  void PrintWordAnalogy(TensorType const &embeddings, std::string const &word1,
                        std::string const &word2, std::string const &word3, SizeType k);

  void PrintKNN(TensorType const &embeddings, std::string const &word0, SizeType k);

  void TestEmbeddings(std::string const &word0, std::string const &word1, std::string const &word2,
                      std::string const &word3, SizeType K);
};

template <class TensorType>
Word2VecClient<TensorType>::Word2VecClient(std::string const &               id,
                                           TrainingParams<TensorType> const &tp,
                                           std::string const &vocab_file, SizeType batch_size,
                                           SizeType number_of_peers)
  : TrainingClient<TensorType>(id, batch_size, tp.starting_learning_rate, number_of_peers)
  , tp_(tp)
  , vocab_file_(vocab_file)
{
  PrepareDataLoader();
  PrepareModel();

  // calc the compatiable linear lr decay
  tp_.learning_rate_param.linear_decay_rate =
      static_cast<DataType>(1) /
      w2v_data_loader_ptr_
          ->EstimatedSampleNumber();  // this decay rate gurantee the lr is reduced to zero by the
  // end of an epoch (despite capping by ending learning rate)
  std::cout << "dataloader_.EstimatedSampleNumber(): "
            << w2v_data_loader_ptr_->EstimatedSampleNumber() << std::endl;

  PrepareOptimiser();
}

template <class TensorType>
void Word2VecClient<TensorType>::PrepareModel()
{
  this->g_ptr_ = std::make_shared<fetch::ml::Graph<TensorType>>();

  this->g_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  this->g_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Context", {});
  this->label_name_ = this->g_ptr_->template AddNode<PlaceHolder<TensorType>>("Label", {});
  skipgram_         = this->g_ptr_->template AddNode<fetch::ml::layers::SkipGram<TensorType>>(
      "SkipGram", {"Input", "Context"}, SizeType(1), SizeType(1), tp_.embedding_size,
      w2v_data_loader_ptr_->vocab_size());

  this->error_name_ = this->g_ptr_->template AddNode<CrossEntropyLoss<TensorType>>(
      "Error", {skipgram_, this->label_name_});

  this->inputs_names_.push_back({"Input", "Context"});
}

template <class TensorType>
void Word2VecClient<TensorType>::PrepareDataLoader()
{

  w2v_data_loader_ptr_ = std::make_shared<fetch::ml::dataloaders::GraphW2VLoader<DataType>>(
      tp_.window_size, tp_.negative_sample_size, tp_.freq_thresh, tp_.max_word_count);
//  w2v_data_loader_ptr_->LoadVocab(vocab_file_);
  w2v_data_loader_ptr_->BuildVocab({ReadFile(vocab_file_)}, tp_.min_count);

  this->dataloader_ptr_ = w2v_data_loader_ptr_;
}

template <class TensorType>
void Word2VecClient<TensorType>::PrepareOptimiser()
{
  // Initialise Optimiser
  this->opti_ptr_ = std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
      std::shared_ptr<fetch::ml::Graph<TensorType>>(this->g_ptr_), this->inputs_names_,
      this->label_name_, this->error_name_, this->learning_rate_);
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void Word2VecClient<TensorType>::Test(DataType &test_loss)
{
  // Implement loss mechanism
  test_loss = static_cast<DataType>(0);

  // Implement test frequency mechanism
  // if (i % tp_.test_frequency == 0)
  //{
  TestEmbeddings(tp_.word0, tp_.word1, tp_.word2, tp_.word3, tp_.k);
  //}
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
    std::cout << "Find word that to " << word3 << " is what " << word2 << " is to " << word1
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
  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<TensorType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<TensorType>>(
          (this->g_ptr_->GetNode(skipgram_))->GetOp());

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<TensorType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  std::cout << std::endl;
  PrintKNN(embeddings->get_weights(), word0, K);
  std::cout << std::endl;
  PrintWordAnalogy(embeddings->get_weights(), word1, word2, word3, K);
}
