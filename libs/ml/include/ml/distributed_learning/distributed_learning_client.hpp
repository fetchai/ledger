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

#include "coordinator.hpp"
#include "core/random.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/optimiser.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <utility>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

template <class TensorType>
class TrainingClient
{
  using DataType         = typename TensorType::Type;
  using SizeType         = typename TensorType::SizeType;
  using VectorTensorType = std::vector<TensorType>;

public:
  TrainingClient(std::string const &id, SizeType batch_size, DataType learning_rate,
                 SizeType number_of_peers);
  virtual ~TrainingClient() = default;

  void SetCoordinator(std::shared_ptr<Coordinator> coordinator_ptr);
  void MainLoop();

  DataType         Train();
  virtual void     Test(DataType &test_loss);
  VectorTensorType GetGradients() const;
  VectorTensorType GetWeights() const;
  void             AddPeers(std::vector<std::shared_ptr<TrainingClient>> const &clients);
  void             BroadcastGradients();
  void             AddGradient(VectorTensorType gradient);
  void             ApplyGradient(VectorTensorType gradients);
  void             SetWeights(VectorTensorType &new_weights);
  virtual void     PrepareModel()      = 0;
  virtual void     PrepareDataLoader() = 0;
  virtual void     PrepareOptimiser()  = 0;

protected:
  // Client's own graph and mutex to protect it's weights
  std::shared_ptr<fetch::ml::Graph<TensorType>> g_ptr_ =
      std::make_shared<fetch::ml::Graph<TensorType>>();
  std::mutex model_mutex_;

  // Client's own dataloader
  std::shared_ptr<fetch::ml::dataloaders::DataLoader<TensorType, TensorType>> dataloader_ptr_;

  // Client's own optimiser
  std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>> opti_ptr_;

  std::vector<std::string> inputs_names_;
  std::string              label_name_;
  std::string              error_name_;

  // Connection to other nodes
  std::vector<std::shared_ptr<TrainingClient>> peers_;

  // Access to coordinator
  std::shared_ptr<Coordinator> coordinator_ptr_;

  // Gradient queue access and mutex to protect it
  std::queue<VectorTensorType> gradient_queue_;
  std::mutex                   queue_mutex_;

  // random number generator for shuffling peers
  fetch::random::LaggedFibonacciGenerator<> gen_;

  // Client id (identification name)
  std::string id_;

  // Learning hyperparameters
  SizeType batch_size_      = 0;
  DataType learning_rate_   = static_cast<DataType>(0);
  SizeType number_of_peers_ = 0;

  void        GetNewGradients(VectorTensorType &new_gradients);
  std::string GetTimeStamp();

  void TrainOnce();
  void TrainWithCoordinator();
  void DoBatch();
};

template <class TensorType>
TrainingClient<TensorType>::TrainingClient(std::string const &id, SizeType batch_size,
                                           DataType learning_rate, SizeType number_of_peers)

  : id_(std::move(id))
  , batch_size_(batch_size)
  , learning_rate_(learning_rate)
  , number_of_peers_(number_of_peers)
{
  // Clear loss file
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::trunc);
  lossfile.close();
}

template <class TensorType>
void TrainingClient<TensorType>::SetCoordinator(std::shared_ptr<Coordinator> coordinator_ptr)
{
  coordinator_ptr_ = coordinator_ptr;
}

/**
 * Main loop that runs in thread
 */
template <class TensorType>
void TrainingClient<TensorType>::MainLoop()
{
  if (coordinator_ptr_->GetMode() == CoordinatorMode::SYNCHRONOUS)
  {
    // Do one batch and end
    TrainOnce();
  }
  else
  {
    // Train batches until coordinator will tell clients to stop
    TrainWithCoordinator();
  }
}

/**
 * Train one batch
 * @return training batch loss
 */
template <class TensorType>
typename TensorType::Type TrainingClient<TensorType>::Train()
{
  dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
  dataloader_ptr_->SetRandomMode(true);

  DataType loss        = static_cast<DataType>(0);
  bool     is_done_set = false;

  std::pair<TensorType, std::vector<TensorType>> input;
  input = dataloader_ptr_->PrepareBatch(batch_size_, is_done_set);

  {
    std::lock_guard<std::mutex> l(model_mutex_);

    // Set inputs and label
    auto input_data_it = input.second.begin();
    auto input_name_it = inputs_names_.begin();

    while (input_name_it != inputs_names_.end())
    {
      g_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    g_ptr_->SetInput(label_name_, input.first);

    TensorType loss_tensor = g_ptr_->ForwardPropagate(error_name_);
    loss                   = *(loss_tensor.begin());
    g_ptr_->BackPropagateError(error_name_);
  }

  return loss;
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void TrainingClient<TensorType>::Test(DataType &test_loss)
{
  dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TEST);

  // Disable random to run model on whole test set
  dataloader_ptr_->SetRandomMode(false);

  SizeType test_set_size = dataloader_ptr_->Size();

  dataloader_ptr_->Reset();
  bool is_done_set;
  auto test_pair = dataloader_ptr_->PrepareBatch(test_set_size, is_done_set);
  {
    std::lock_guard<std::mutex> l(model_mutex_);

    // Set inputs and label
    auto input_data_it = test_pair.second.begin();
    auto input_name_it = inputs_names_.begin();

    while (input_name_it != inputs_names_.end())
    {
      g_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    g_ptr_->SetInput(label_name_, test_pair.first);

    test_loss = *(g_ptr_->Evaluate(error_name_).begin());
  }
}

/**
 * @return vector of gradient update values
 */
template <class TensorType>
std::vector<TensorType> TrainingClient<TensorType>::GetGradients() const
{
  std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
  return g_ptr_->GetGradients();
}

/**
 * @return vector of weights that represents the model
 */
template <class TensorType>
std::vector<TensorType> TrainingClient<TensorType>::GetWeights() const
{
  std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
  return g_ptr_->get_weights();
}

/**
 * Add pointers to other clients
 * @param clients
 */
template <class TensorType>
void TrainingClient<TensorType>::AddPeers(
    std::vector<std::shared_ptr<TrainingClient>> const &clients)
{
  for (auto const &p : clients)
  {
    if (p.get() != this)
    {
      peers_.push_back(p);
    }
  }
}

/**
 * Adds own gradient to peers queues
 */
template <class TensorType>
void TrainingClient<TensorType>::BroadcastGradients()
{
  // Load own gradient
  VectorTensorType current_gradient = g_ptr_->GetGradients();

  // Give gradients to peers
  for (SizeType i{0}; i < number_of_peers_; ++i)
  {
    peers_[i]->AddGradient(current_gradient);
  }
}

/**
 * Adds gradient to own gradient queue
 * @param gradient
 */
template <class TensorType>
void TrainingClient<TensorType>::AddGradient(VectorTensorType gradient)
{
  {
    std::lock_guard<std::mutex> l(queue_mutex_);
    gradient_queue_.push(gradient);
  }
}

/**
 * Applies gradient multiplied by -LEARNING_RATE
 * @param gradients
 */
template <class TensorType>
void TrainingClient<TensorType>::ApplyGradient(VectorTensorType gradients)
{
  // Step of SGD optimiser
  for (SizeType j{0}; j < gradients.size(); j++)
  {
    fetch::math::Multiply(gradients.at(j), -learning_rate_, gradients.at(j));
  }

  // Apply gradients to own model
  {
    std::lock_guard<std::mutex> l(model_mutex_);
    g_ptr_->ApplyGradients(gradients);
  }
}

/**
 * Rewrites current model with given one
 * @param new_weights Vector of weights that represent model
 */
template <class TensorType>
void TrainingClient<TensorType>::SetWeights(VectorTensorType &new_weights)
{
  std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
  g_ptr_->SetWeights(new_weights);
}

template <class TensorType>
void TrainingClient<TensorType>::GetNewGradients(VectorTensorType &new_gradients)
{
  std::lock_guard<std::mutex> l(queue_mutex_);
  new_gradients = gradient_queue_.front();
  gradient_queue_.pop();
}

// Timestamp for logging
template <class TensorType>
std::string TrainingClient<TensorType>::GetTimeStamp()
{
  // TODO(1564): Implement timestamp
  return "TIMESTAMP";
}

/**
 * Do one batch training, run model on test set and write loss to csv file
 */
template <class TensorType>
void TrainingClient<TensorType>::TrainOnce()
{
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::app);
  DataType      loss;

  DoBatch();

  // Validate loss for logging purpose
  Test(loss);

  // Save loss variation data
  // Upload to https://plot.ly/create/#/ for visualisation
  if (lossfile)
  {
    lossfile << GetTimeStamp() << ", " << loss << "\n";
  }

  lossfile << GetTimeStamp() << ", "
           << "STOPPED"
           << "\n";
  lossfile.close();
}

/**
 * Do batch training repeatedly while coordinator state is set to RUN
 */
template <class TensorType>
void TrainingClient<TensorType>::TrainWithCoordinator()
{
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::app);
  DataType      loss;

  while (coordinator_ptr_->GetState() == CoordinatorState::RUN)
  {
    DoBatch();
    coordinator_ptr_->IncrementIterationsCounter();

    // Validate loss for logging purpose
    Test(loss);

    // Save loss variation data
    // Upload to https://plot.ly/create/#/ for visualisation
    if (lossfile)
    {
      lossfile << GetTimeStamp() << ", " << loss << "\n";
    }
  }

  lossfile << GetTimeStamp() << ", "
           << "STOPPED"
           << "\n";
  lossfile.close();
}

/**
 * Do one batch and broadcast gradients
 */
template <class TensorType>
void TrainingClient<TensorType>::DoBatch()
{
  // Train one batch to create own gradient
  Train();

  // Interaction with peers is skipped in synchronous mode
  if (coordinator_ptr_->GetMode() != CoordinatorMode::SYNCHRONOUS)
  {
    // Shuffle the peers list to get new contact for next update
    fetch::random::Shuffle(gen_, peers_, peers_);

    // Put own gradient to peers queues
    BroadcastGradients();

    // Load own gradient
    VectorTensorType new_gradients;

    // Sum all gradient in queue
    while (!gradient_queue_.empty())
    {
      GetNewGradients(new_gradients);

      g_ptr_->AddExternalGradients(new_gradients);
    }
  }

  // Apply sum of all gradients from queue along with own gradient
  {
    std::lock_guard<std::mutex> l(model_mutex_);
    opti_ptr_->ApplyGradients(batch_size_);
  }
}
