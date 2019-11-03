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

#include "dmlf/distributed_learning/distributed_learning_client.hpp"
#include "dmlf/distributed_learning/translator.hpp"
#include "dmlf/distributed_learning/word2vec_training_params.hpp"
#include "math/clustering/knn.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/utilities/word2vec_utilities.hpp"

namespace fetch {
namespace dmlf {
namespace distributed_learning {

template <class TensorType>
class Word2VecClient : public TrainingClient<TensorType>
{
  using DataType         = typename TensorType::Type;
  using SizeType         = fetch::math::SizeType;
  using VectorTensorType = std::vector<TensorType>;
  using GradientType     = fetch::dmlf::Update<TensorType>;

public:
  Word2VecClient(std::string const &id, Word2VecTrainingParams<DataType> const &tp,
                 std::shared_ptr<std::mutex> console_mutex_ptr);

  void Run() override;

  void Test() override;

  float GetAnalogyScore();

  std::shared_ptr<GradientType> GetGradients() override;

  std::pair<std::vector<std::string>, byte_array::ConstByteArray> GetVocab();
  void AddVocab(const std::pair<std::vector<std::string>, byte_array::ConstByteArray> &vocab_info);

  std::pair<TensorType, TensorType> TranslateWeights(TensorType &                      new_weights,
                                                     const byte_array::ConstByteArray &vocab_hash);

private:
  Word2VecTrainingParams<DataType>                                    tp_;
  std::string                                                         skipgram_;
  std::shared_ptr<fetch::ml::dataloaders::GraphW2VLoader<TensorType>> w2v_data_loader_ptr_;
  float                                                               analogy_score_ = 0.0f;
  Translator                                                          translator_;

  void PrepareOptimiser();

  VectorTensorType TranslateGradients(std::shared_ptr<GradientType> &new_gradients) override;

  float ComputeAnalogyScore();
};

template <class TensorType>
Word2VecClient<TensorType>::Word2VecClient(std::string const &                     id,
                                           Word2VecTrainingParams<DataType> const &tp,
                                           std::shared_ptr<std::mutex> console_mutex_ptr)
  : TrainingClient<TensorType>(id, tp, console_mutex_ptr)
  , tp_(tp)
{
  // set up dataloader
  w2v_data_loader_ptr_ = std::make_shared<fetch::ml::dataloaders::GraphW2VLoader<TensorType>>(
      tp_.window_size, tp_.negative_sample_size, tp_.freq_thresh, tp_.max_word_count);
  this->dataloader_ptr_ = w2v_data_loader_ptr_;

  // build vocab
  w2v_data_loader_ptr_->BuildVocabAndData({tp_.data}, tp_.min_count);

  // calculate the compatible linear lr decay
  // this decay rate guarantees that the lr is reduced to zero by the
  // end of an epoch (despite capping by ending learning rate)
  DataType est_samples                      = w2v_data_loader_ptr_->EstimatedSampleNumber();
  tp_.learning_rate_param.linear_decay_rate = DataType{1} / est_samples;
  std::cout << "id: " << id << ", dataloader_.EstimatedSampleNumber(): " << est_samples
            << std::endl;

  PrepareOptimiser();

  translator_.SetMyVocab(w2v_data_loader_ptr_->GetVocab());
}

/**
 * Main loop that runs in thread
 */
template <class TensorType>
void Word2VecClient<TensorType>::Run()
{
  TrainingClient<TensorType>::Run();
  analogy_score_ = ComputeAnalogyScore();
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void Word2VecClient<TensorType>::Test()
{
  if (this->batch_counter_ % tp_.test_frequency == tp_.test_frequency - 1)
  {
    // Lock model
    FETCH_LOCK(this->model_mutex_);

    fetch::ml::utilities::TestEmbeddings<TensorType>(
        *this->graph_ptr_, skipgram_, *w2v_data_loader_ptr_, tp_.word0, tp_.word1, tp_.word2,
        tp_.word3, tp_.k, tp_.analogies_test_file, false, "/tmp/w2v_client_" + this->id_);
  }
}

template <class TensorType>
float Word2VecClient<TensorType>::ComputeAnalogyScore()
{
  TensorType const &weights = fetch::ml::utilities::GetEmbeddings(*this->graph_ptr_, skipgram_);

  return fetch::ml::utilities::AnalogiesFileTest(*w2v_data_loader_ptr_, weights,
                                                 tp_.analogies_test_file)
      .second;
}

/**
 * @return vector of gradient update values
 */
template <class TensorType>
std::shared_ptr<fetch::dmlf::Update<TensorType>> Word2VecClient<TensorType>::GetGradients()
{
  FETCH_LOCK(this->model_mutex_);
  return std::make_shared<GradientType>(this->graph_ptr_->GetGradients(),
                                        w2v_data_loader_ptr_->GetVocabHash(),
                                        w2v_data_loader_ptr_->GetVocab()->GetReverseVocab());
}

/**
 * @return pair of vocab strings and vocab hash
 */
template <class TensorType>
std::pair<std::vector<std::string>, byte_array::ConstByteArray>
Word2VecClient<TensorType>::GetVocab()
{
  auto vocab      = w2v_data_loader_ptr_->GetVocab();
  auto vocab_hash = w2v_data_loader_ptr_->GetVocabHash();
  // "ReverseVocab" is a vector of strings, and is the most compact way of sending the vocab
  return std::make_pair(vocab->GetReverseVocab(), vocab->GetVocabHash());
}

/**
 * Add a vocab to the translator_
 * @param vocab_info pair of vocab strings and vocab hash
 */
template <class TensorType>
void Word2VecClient<TensorType>::AddVocab(
    std::pair<std::vector<std::string>, byte_array::ConstByteArray> const &vocab_info)
{
  translator_.AddVocab(vocab_info.second, vocab_info.first);
}

template <class TensorType>
std::pair<TensorType, TensorType> Word2VecClient<TensorType>::TranslateWeights(
    TensorType &new_weights, const byte_array::ConstByteArray &vocab_hash)
{
  std::pair<TensorType, TensorType> ret =
      translator_.Translate<TensorType>(new_weights, vocab_hash);

  return ret;
}

// private

template <class TensorType>
void Word2VecClient<TensorType>::PrepareOptimiser()
{
  // set up the graph first
  this->graph_ptr_ = std::make_shared<fetch::ml::Graph<TensorType>>();
  std::string input_name =
      this->graph_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string context_name =
      this->graph_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Context", {});
  this->label_name_ =
      this->graph_ptr_->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});
  skipgram_ = this->graph_ptr_->template AddNode<fetch::ml::layers::SkipGram<TensorType>>(
      "SkipGram", {input_name, context_name}, SizeType(1), SizeType(1), tp_.embedding_size,
      w2v_data_loader_ptr_->vocab_size());

  this->error_name_ =
      this->graph_ptr_->template AddNode<fetch::ml::ops::CrossEntropyLoss<TensorType>>(
          "Error", {skipgram_, this->label_name_});

  this->inputs_names_ = {input_name, context_name};

  // Initialise Optimiser
  this->optimiser_ptr_ = std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
      this->graph_ptr_, this->inputs_names_, this->label_name_, this->error_name_,
      tp_.learning_rate_param);
}

template <class TensorType>
typename Word2VecClient<TensorType>::VectorTensorType
Word2VecClient<TensorType>::TranslateGradients(
    std::shared_ptr<Word2VecClient::GradientType> &new_gradients)
{
  assert(new_gradients->GetGradients().size() ==
         2);  // Translation unit is only defined for word2vec

  // Add vocab from update if not known by translator
  if (!translator_.VocabKnown(new_gradients->GetHash()))
  {
    translator_.AddVocab(new_gradients->GetHash(), new_gradients->GetReverseVocab());
  }

  VectorTensorType ret;
  ret.push_back(
      translator_
          .Translate<TensorType>(new_gradients->GetGradients().at(0), new_gradients->GetHash())
          .first);
  ret.push_back(
      translator_
          .Translate<TensorType>(new_gradients->GetGradients().at(1), new_gradients->GetHash())
          .first);
  return ret;
}

template <class TensorType>
float Word2VecClient<TensorType>::GetAnalogyScore()
{
  return analogy_score_;
}

}  // namespace distributed_learning
}  // namespace dmlf
}  // namespace fetch
