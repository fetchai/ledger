#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "dmlf/collective_learning/client_algorithm.hpp"
#include "dmlf/collective_learning/client_algorithm_controller.hpp"
#include "dmlf/collective_learning/translator.hpp"
#include "dmlf/collective_learning/word2vec_training_params.hpp"
#include "math/clustering/knn.hpp"
#include "ml/optimisation/lazy_adam_optimiser.hpp"
#include "ml/utilities/word2vec_utilities.hpp"

namespace fetch {
namespace dmlf {
namespace collective_learning {

template <class TensorType>
class ClientWord2VecAlgorithm : public ClientAlgorithm<TensorType>
{
  using DataType         = typename TensorType::Type;
  using SizeType         = fetch::math::SizeType;
  using VectorTensorType = std::vector<TensorType>;
  using VectorSizeVector = std::vector<std::vector<SizeType>>;

  using GradientType               = fetch::dmlf::deprecated_Update<TensorType>;
  using AlgorithmControllerType    = ClientAlgorithmController<TensorType>;
  using AlgorithmControllerPtrType = std::shared_ptr<ClientAlgorithmController<TensorType>>;

public:
  ClientWord2VecAlgorithm(AlgorithmControllerPtrType algorithm_controller, std::string const &id,
                          Word2VecTrainingParams<DataType> const &tp,
                          std::shared_ptr<std::mutex>             console_mutex_ptr);

  void Run() override;

  void Test() override;

  float GetAnalogyScore();

  std::shared_ptr<GradientType> GetUpdate() override;

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

  VectorSizeVector TranslateUpdate(std::shared_ptr<GradientType> &new_gradients) override;

  float ComputeAnalogyScore();
};

template <class TensorType>
ClientWord2VecAlgorithm<TensorType>::ClientWord2VecAlgorithm(
    AlgorithmControllerPtrType algorithm_controller, std::string const &id,
    Word2VecTrainingParams<DataType> const &tp, std::shared_ptr<std::mutex> console_mutex_ptr)
  : ClientAlgorithm<TensorType>(algorithm_controller, id, tp, console_mutex_ptr)
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
void ClientWord2VecAlgorithm<TensorType>::Run()
{
  ClientAlgorithm<TensorType>::Run();
  analogy_score_ = ComputeAnalogyScore();
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void ClientWord2VecAlgorithm<TensorType>::Test()
{
  if (this->update_counter_ % tp_.test_frequency == tp_.test_frequency - 1)
  {
    // Lock model
    FETCH_LOCK(this->model_mutex_);

    fetch::ml::utilities::TestEmbeddings<TensorType>(
        *this->graph_ptr_, skipgram_, *(w2v_data_loader_ptr_->GetVocab()), tp_.word0, tp_.word1,
        tp_.word2, tp_.word3, tp_.k, tp_.analogies_test_file, false,
        "/tmp/w2v_client_" + this->id_);
  }
}

template <class TensorType>
float ClientWord2VecAlgorithm<TensorType>::ComputeAnalogyScore()
{
  TensorType const &weights = fetch::ml::utilities::GetEmbeddings(*this->graph_ptr_, skipgram_);

  return fetch::ml::utilities::AnalogiesFileTest(*(w2v_data_loader_ptr_->GetVocab()), weights,
                                                 tp_.analogies_test_file)
      .second;
}

/**
 * @return vector of gradient update values
 */
template <class TensorType>
std::shared_ptr<fetch::dmlf::deprecated_Update<TensorType>>
ClientWord2VecAlgorithm<TensorType>::GetUpdate()
{
  FETCH_LOCK(this->model_mutex_);

  std::vector<std::unordered_set<SizeType>> vector_set =
      this->graph_ptr_->GetUpdatedRowsReferences();
  std::vector<TensorType> vector_tensor = this->graph_ptr_->GetGradients();

  // Return update values
  std::vector<std::vector<SizeType>> out_vector;
  std::vector<TensorType>            out_tensors;

  for (SizeType i{0}; i < vector_set.size(); i++)
  {
    // Convert unordered_set to vector
    out_vector.emplace_back(
        std::vector<SizeType>(vector_set.at(i).begin(), vector_set.at(i).end()));
    // Sparsify gradient tensors to contain only updated rows
    out_tensors.emplace_back(fetch::ml::utilities::ToSparse(vector_tensor.at(i), vector_set.at(i)));
  }

  return std::make_shared<GradientType>(out_tensors, w2v_data_loader_ptr_->GetVocabHash(),
                                        w2v_data_loader_ptr_->GetVocab()->GetReverseVocab(),
                                        out_vector);
}

/**
 * @return pair of vocab strings and vocab hash
 */
template <class TensorType>
std::pair<std::vector<std::string>, byte_array::ConstByteArray>
ClientWord2VecAlgorithm<TensorType>::GetVocab()
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
void ClientWord2VecAlgorithm<TensorType>::AddVocab(
    std::pair<std::vector<std::string>, byte_array::ConstByteArray> const &vocab_info)
{
  translator_.AddVocab(vocab_info.second, vocab_info.first);
}

template <class TensorType>
std::pair<TensorType, TensorType> ClientWord2VecAlgorithm<TensorType>::TranslateWeights(
    TensorType &new_weights, const byte_array::ConstByteArray &vocab_hash)
{
  std::pair<TensorType, TensorType> ret =
      translator_.TranslateWeights<TensorType>(new_weights, vocab_hash);

  return ret;
}

// private

template <class TensorType>
void ClientWord2VecAlgorithm<TensorType>::PrepareOptimiser()
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

  this->input_names_ = {input_name, context_name};

  // Initialise Optimiser
  this->optimiser_ptr_ = std::make_shared<fetch::ml::optimisers::LazyAdamOptimiser<TensorType>>(
      this->graph_ptr_, this->input_names_, this->label_name_, this->error_name_,
      tp_.learning_rate_param);
}

template <class TensorType>
typename ClientWord2VecAlgorithm<TensorType>::VectorSizeVector
ClientWord2VecAlgorithm<TensorType>::TranslateUpdate(
    std::shared_ptr<ClientWord2VecAlgorithm::GradientType> &new_gradients)
{
  assert(new_gradients->GetGradients().size() ==
         2);  // Translation unit is only defined for word2vec

  // Add vocab from update if not known by translator
  if (!translator_.VocabKnown(new_gradients->GetHash()))
  {
    translator_.AddVocab(new_gradients->GetHash(), new_gradients->GetReverseVocab());
  }

  VectorSizeVector translated_rows_updates;

  translated_rows_updates.emplace_back(translator_.TranslateUpdate<TensorType>(
      new_gradients->GetUpdatedRows().at(0), new_gradients->GetHash()));
  translated_rows_updates.emplace_back(translator_.TranslateUpdate<TensorType>(
      new_gradients->GetUpdatedRows().at(1), new_gradients->GetHash()));

  return translated_rows_updates;
}

template <class TensorType>
float ClientWord2VecAlgorithm<TensorType>::GetAnalogyScore()
{
  return analogy_score_;
}

}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
