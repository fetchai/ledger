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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "dmlf/distributed_learning/client_params.hpp"
#include "dmlf/networkers/abstract_learner_networker.hpp"
#include "dmlf/update.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/model/sequential.hpp"
#include "ml/utilities/utils.hpp"

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fetch {
namespace dmlf {
namespace distributed_learning {

template <class TensorType>
class TrainingClient
{
  using DataType         = typename TensorType::Type;
  using SizeType         = fetch::math::SizeType;
  using VectorTensorType = std::vector<TensorType>;
  using TimestampType    = int64_t;
  using UpdateType       = fetch::dmlf::Update<TensorType>;
  using DataloaderPtrType =
      std::shared_ptr<fetch::ml::dataloaders::DataLoader<TensorType, TensorType>>;
  using GraphPtrType     = std::shared_ptr<fetch::ml::Graph<TensorType>>;
  using OptimiserPtrType = std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>>;
  using ModelPtrType     = std::shared_ptr<fetch::ml::model::Sequential<TensorType>>;

public:
  TrainingClient(std::string id, ClientParams<DataType> const &client_params,
                 std::shared_ptr<std::mutex> console_mutex_ptr);

  TrainingClient(std::string id, ModelPtrType model_ptr,
                 ClientParams<DataType> const &client_params,
                 std::shared_ptr<std::mutex>   console_mutex_ptr);

  virtual ~TrainingClient() = default;

  void SetNetworker(std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker> networker_ptr);

  virtual void Run();

  void Train();

  virtual void Test();

  virtual std::shared_ptr<UpdateType> GetGradients();

  VectorTensorType GetWeights() const;

  void SetWeights(VectorTensorType const &new_weights);

  void SetParams(ClientParams<DataType> const &new_params);

  ModelPtrType GetModel();

  std::string GetId() const;

  void ResetLossCnt();

  DataType GetLossAverage() const;

protected:
  // Client id (identification name)
  std::string id_;

  // Latest loss
  DataType train_loss_ = fetch::math::numeric_max<DataType>();
  DataType test_loss_  = fetch::math::numeric_max<DataType>();

  DataType train_loss_sum_ = static_cast<DataType>(0);
  SizeType train_loss_cnt_ = 0;

  // Client's own model and mutex to protect its weights
  ModelPtrType       model_ptr_;
  GraphPtrType       graph_ptr_;
  OptimiserPtrType   optimiser_ptr_;
  DataloaderPtrType  dataloader_ptr_;
  mutable std::mutex model_mutex_;

  std::vector<std::string> inputs_names_;
  std::string              label_name_;
  std::string              error_name_;

  // Pointer to client's own iLearnerNetworker
  std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker> networker_ptr_;

  // Console mutex pointer
  std::shared_ptr<std::mutex> console_mutex_ptr_;

  // Learning hyperparameters
  SizeType batch_size_    = 0;
  DataType learning_rate_ = static_cast<DataType>(0);

  // Count for number of batches
  SizeType batch_counter_  = 0;
  SizeType update_counter_ = 0;
  SizeType max_updates_    = 0;

  // Print to console flag
  bool print_loss_;

  virtual VectorTensorType TranslateGradients(std::shared_ptr<UpdateType> &new_gradients);

  TimestampType GetTimestamp();

  void DoBatch();

  void ClearLossFile();

private:
  void GraphAddGradients(VectorTensorType const &gradients);
};

template <class TensorType>
TrainingClient<TensorType>::TrainingClient(std::string id, ModelPtrType model_ptr,
                                           ClientParams<DataType> const &client_params,
                                           std::shared_ptr<std::mutex>   console_mutex_ptr)
  : id_(std::move(id))
  , model_ptr_(std::move(model_ptr))
  , console_mutex_ptr_(std::move(console_mutex_ptr))
{
  dataloader_ptr_ = model_ptr_->dataloader_ptr_;
  graph_ptr_      = model_ptr_->graph_ptr_;
  optimiser_ptr_  = model_ptr_->optimiser_ptr_;

  SetParams(client_params);
  ClearLossFile();
}

template <class TensorType>
TrainingClient<TensorType>::TrainingClient(std::string                   id,
                                           ClientParams<DataType> const &client_params,
                                           std::shared_ptr<std::mutex>   console_mutex_ptr)
  : id_(std::move(id))
  , console_mutex_ptr_(std::move(console_mutex_ptr))
{
  SetParams(client_params);
  ClearLossFile();
}

template <class TensorType>
void TrainingClient<TensorType>::ClearLossFile()
{
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::trunc);
  lossfile.close();
}

template <class TensorType>
void TrainingClient<TensorType>::SetParams(
    ClientParams<typename TensorType::Type> const &new_params)
{
  inputs_names_  = new_params.inputs_names;
  label_name_    = new_params.label_name;
  error_name_    = new_params.error_name;
  batch_size_    = new_params.batch_size;
  learning_rate_ = new_params.learning_rate;
  print_loss_    = new_params.print_loss;
  max_updates_   = new_params.max_updates;
}

template <class TensorType>
void TrainingClient<TensorType>::SetNetworker(
    std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker> networker_ptr)
{
  networker_ptr_ = networker_ptr;
}

template <class TensorType>
std::string TrainingClient<TensorType>::GetId() const
{
  return id_;
}

template <class TensorType>
void TrainingClient<TensorType>::ResetLossCnt()
{
  train_loss_sum_ = static_cast<DataType>(0);
  train_loss_cnt_ = 0;
}

template <class TensorType>
typename TensorType::Type TrainingClient<TensorType>::GetLossAverage() const
{
  return train_loss_sum_ / static_cast<DataType>(train_loss_cnt_);
}

/**
 * Main loop that runs in thread
 */
template <class TensorType>
void TrainingClient<TensorType>::Run()
{
  ResetLossCnt();

  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::app);

  update_counter_ = 0;
  while (update_counter_ < max_updates_)
  {
    DoBatch();

    // Validate loss for logging purpose
    Test();

    // Save loss variation data
    // Upload to https://plot.ly/create/#/ for visualisation

    if (lossfile)
    {
      lossfile << fetch::ml::utilities::GetStrTimestamp() << ", "
               << static_cast<double>(train_loss_) << ", " << static_cast<double>(test_loss_)
               << "\n";
    }

    if (print_loss_)
    {
      // Lock console
      FETCH_LOCK(*console_mutex_ptr_);
      std::cout << "Client " << id_ << "\tTraining loss: " << static_cast<double>(train_loss_)
                << "\tTest loss: " << static_cast<double>(test_loss_) << std::endl;
    }
  }

  optimiser_ptr_->IncrementEpochCounter();
  optimiser_ptr_->UpdateLearningRate();

  if (lossfile)
  {
    lossfile << fetch::ml::utilities::GetStrTimestamp() << ", "
             << "STOPPED"
             << "\n";
    lossfile.close();
  }

  if (print_loss_)
  {
    // Lock console
    FETCH_LOCK(*console_mutex_ptr_);
    std::cout << "Client " << id_ << " STOPPED" << std::endl;
  }
}

/**
 * Train one batch
 * @return training batch loss
 */
template <class TensorType>
void TrainingClient<TensorType>::Train()
{
  dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
  dataloader_ptr_->SetRandomMode(true);

  bool is_done_set = false;

  std::pair<TensorType, std::vector<TensorType>> input;
  input = dataloader_ptr_->PrepareBatch(batch_size_, is_done_set);
  {
    FETCH_LOCK(model_mutex_);

    // Set inputs and label
    auto input_data_it = input.second.begin();
    auto input_name_it = inputs_names_.begin();

    while (input_name_it != inputs_names_.end())
    {
      graph_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    graph_ptr_->SetInput(label_name_, input.first);

    TensorType loss_tensor = graph_ptr_->ForwardPropagate(error_name_);
    train_loss_            = *(loss_tensor.begin());

    train_loss_sum_ += train_loss_;
    train_loss_cnt_++;

    graph_ptr_->BackPropagate(error_name_);
  }
  update_counter_++;
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void TrainingClient<TensorType>::Test()
{
  // If test set is not available we run test on whole training set
  if (dataloader_ptr_->IsModeAvailable(fetch::ml::dataloaders::DataLoaderMode::TEST))
  {
    dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TEST);
  }
  else
  {
    dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
  }

  // Disable random to run model on whole test set
  dataloader_ptr_->SetRandomMode(false);

  SizeType test_set_size = dataloader_ptr_->Size();

  dataloader_ptr_->Reset();
  bool is_done_set;
  auto test_pair = dataloader_ptr_->PrepareBatch(test_set_size, is_done_set);
  {
    FETCH_LOCK(model_mutex_);

    // Set inputs and label
    auto input_data_it = test_pair.second.begin();
    auto input_name_it = inputs_names_.begin();

    while (input_name_it != inputs_names_.end())
    {
      graph_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    graph_ptr_->SetInput(label_name_, test_pair.first);

    test_loss_ = *(graph_ptr_->Evaluate(error_name_).begin());
  }
  dataloader_ptr_->Reset();
}

/**
 * @return vector of gradient update values
 */
template <class TensorType>
std::shared_ptr<typename TrainingClient<TensorType>::UpdateType>
TrainingClient<TensorType>::GetGradients()
{
  FETCH_LOCK(model_mutex_);
  return std::make_shared<UpdateType>(graph_ptr_->GetGradients());
}

/**
 * @return vector of weights that represents the model
 */
template <class TensorType>
std::vector<TensorType> TrainingClient<TensorType>::GetWeights() const
{
  FETCH_LOCK(model_mutex_);
  return graph_ptr_->GetWeightsReferences();
}

/**
 * Rewrites current model with given one
 * @param new_weights Vector of weights that represent model
 */
template <class TensorType>
void TrainingClient<TensorType>::SetWeights(VectorTensorType const &new_weights)
{
  FETCH_LOCK(model_mutex_);

  auto weights_it = new_weights.cbegin();
  for (auto &trainable_lookup : graph_ptr_->trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Trainable<TensorType>>(
        (trainable_lookup.second)->GetOp());
    trainable_ptr->SetWeights(*weights_it);
    ++weights_it;
  }
}

template <class TensorType>
std::vector<TensorType> TrainingClient<TensorType>::TranslateGradients(
    std::shared_ptr<UpdateType> &new_gradients)
{
  return new_gradients->GetGradients();
}

// Timestamp for gradient queue
template <class TensorType>
int64_t TrainingClient<TensorType>::GetTimestamp()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

/**
 * Do one batch and broadcast gradients
 */
template <class TensorType>
void TrainingClient<TensorType>::DoBatch()
{
  // Train one batch to create own gradient
  Train();

  // Push own gradient to iLearner
  auto update = GetGradients();
  networker_ptr_->PushUpdate(update);

  SizeType ucnt = 0;

  // Sum all gradient from iLearner
  while (networker_ptr_->GetUpdateCount() > 0)
  {
    ucnt++;
    auto             new_update    = networker_ptr_->GetUpdate<UpdateType>();
    VectorTensorType new_gradients = TranslateGradients(new_update);

    GraphAddGradients(new_gradients);
    update_counter_++;
  }

  // Apply sum of all gradients from queue along with own gradient
  {
    FETCH_LOCK(model_mutex_);
    optimiser_ptr_->ApplyGradients(batch_size_);
    optimiser_ptr_->IncrementBatchCounters(batch_size_);
    optimiser_ptr_->UpdateLearningRate();
  }
  batch_counter_++;
}

///////////////////////
/// private methods ///
///////////////////////

/**
 * Adds a vector of Tensors to the gradient accumulation of all the trainable pointers in the graph.
 * @param gradient
 */
template <class TensorType>
void TrainingClient<TensorType>::GraphAddGradients(VectorTensorType const &gradients)
{
  assert(gradients.size() == graph_ptr_->GetTrainables().size());
  auto grad_it = gradients.begin();
  for (auto &trainable : graph_ptr_->GetTrainables())
  {
    auto weights_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(trainable);
    weights_ptr->AddToGradient(*grad_it);
    ++grad_it;
  }
}

template <class TensorType>
typename TrainingClient<TensorType>::ModelPtrType TrainingClient<TensorType>::GetModel()
{
  return model_ptr_;
}

}  // namespace distributed_learning
}  // namespace dmlf
}  // namespace fetch
