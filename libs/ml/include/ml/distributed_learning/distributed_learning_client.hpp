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
#include "core/mutex.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/optimiser.hpp"

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace distributed_learning {

template <typename DataType>
struct ClientParams
{
  using SizeType = fetch::math::SizeType;

  SizeType batch_size{};
  DataType learning_rate;
  bool     print_loss = false;

  std::vector<std::string> inputs_names = {"Input"};
  std::string              label_name   = "Label";
  std::string              error_name   = "Error";
};

template <class TensorType>
class TrainingClient
{
  using DataType         = typename TensorType::Type;
  using SizeType         = typename TensorType::SizeType;
  using VectorTensorType = std::vector<TensorType>;
  using TimestampType    = int64_t;
  using GradientType     = std::pair<VectorTensorType, TimestampType>;
  using GraphPtrType     = std::shared_ptr<fetch::ml::Graph<TensorType>>;

public:
  TrainingClient(std::string id, ClientParams<DataType> const &client_params,
                 std::shared_ptr<std::mutex> console_mutex_ptr);

  TrainingClient(
      std::string id, GraphPtrType graph_ptr,
      std::shared_ptr<fetch::ml::dataloaders::DataLoader<TensorType, TensorType>> loader_ptr,
      std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>>               optimiser_ptr,
      ClientParams<DataType> const &client_params, std::shared_ptr<std::mutex> console_mutex_ptr);

  virtual ~TrainingClient()
  {
    export_stopped_ = true;
    export_buffer_cv_.notify_all();
  }

  void SetCoordinator(std::shared_ptr<Coordinator<TensorType>> coordinator_ptr);

  void Run();

  void Train();

  virtual void Test();

  VectorTensorType GetGradients() const;

  VectorTensorType GetWeights() const;

  void AddExportGradient(GradientType &gradient);

  void SetWeights(VectorTensorType const &new_weights);

  void SetParams(ClientParams<DataType> const &new_params);

  GraphPtrType GetModel();

  std::string GetId() const;

protected:
  // Client id (identification name)
  std::string id_;

  // Latest loss
  DataType train_loss_ = fetch::math::numeric_max<DataType>();
  DataType test_loss_  = fetch::math::numeric_max<DataType>();

  // Client's own graph and mutex to protect its weights
  GraphPtrType       g_ptr_;
  mutable std::mutex model_mutex_;

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
  std::shared_ptr<Coordinator<TensorType>> coordinator_ptr_;

  // Gradient queue access and mutex to protect it
  std::queue<GradientType> gradient_queue_;
  mutable std::mutex       queue_mutex_;

  // Export buffer logic
  std::queue<GradientType>     export_buffer_;
  mutable std::mutex           export_buffer_mutex_;
  std::condition_variable      export_buffer_cv_;
  mutable std::mutex           export_buffer_cv_mutex_;
  std::shared_ptr<std::thread> export_buffer_thread_;
  bool                         export_stopped_ = false;

  // Console mutex pointer
  std::shared_ptr<std::mutex> console_mutex_ptr_;

  // Learning hyperparameters
  SizeType batch_size_    = 0;
  DataType learning_rate_ = static_cast<DataType>(0);

  // Count for number of batches
  SizeType batch_counter_ = 0;

  // Print to console flag
  bool print_loss_;

  void GetNewGradients(VectorTensorType &new_gradients);

  std::string   GetStrTimestamp();
  TimestampType GetTimestamp();

  void TrainOnce();

  void TrainWithCoordinator();

  void DoBatch();

  void ClearLossFile();

  void ExportBufferLoop();

  void Initialise();

private:
  void QueueAddGradient(GradientType &gradient);
  void GraphAddGradients(GraphPtrType g_ptr, VectorTensorType const &gradients);
};

template <class TensorType>
TrainingClient<TensorType>::TrainingClient(
    std::string id, GraphPtrType graph_ptr,
    std::shared_ptr<fetch::ml::dataloaders::DataLoader<TensorType, TensorType>> loader_ptr,
    std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>>               optimiser_ptr,
    ClientParams<DataType> const &client_params, std::shared_ptr<std::mutex> console_mutex_ptr)
  : id_(std::move(id))
  , g_ptr_(std::move(graph_ptr))
  , dataloader_ptr_(std::move(loader_ptr))
  , opti_ptr_(std::move(optimiser_ptr))
  , console_mutex_ptr_(std::move(console_mutex_ptr))
{
  SetParams(client_params);
  ClearLossFile();
  Initialise();
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
  Initialise();
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
}

template <class TensorType>
void TrainingClient<TensorType>::SetCoordinator(
    std::shared_ptr<Coordinator<TensorType>> coordinator_ptr)
{
  coordinator_ptr_ = coordinator_ptr;
}

template <class TensorType>
std::string TrainingClient<TensorType>::GetId() const
{
  return id_;
}

/**
 * Main loop that runs in thread
 */
template <class TensorType>
void TrainingClient<TensorType>::Run()
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
      g_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    g_ptr_->SetInput(label_name_, input.first);

    TensorType loss_tensor = g_ptr_->ForwardPropagate(error_name_);
    train_loss_            = *(loss_tensor.begin());
    g_ptr_->BackPropagate(error_name_);
    std::cout << id_ << " train loss: " << train_loss_ << std::endl;
  }
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
      g_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    g_ptr_->SetInput(label_name_, test_pair.first);

    test_loss_ = *(g_ptr_->Evaluate(error_name_).begin());
  }
  dataloader_ptr_->Reset();
}

/**
 * @return vector of gradient update values
 */
template <class TensorType>
std::vector<TensorType> TrainingClient<TensorType>::GetGradients() const
{
  FETCH_LOCK(model_mutex_);
  return g_ptr_->GetGradients();
}

/**
 * @return vector of weights that represents the model
 */
template <class TensorType>
std::vector<TensorType> TrainingClient<TensorType>::GetWeights() const
{
  FETCH_LOCK(model_mutex_);
  return g_ptr_->GetWeightsReferences();
}

/**
 * Adds gradient to export queue
 * @param gradient
 */
template <class TensorType>
void TrainingClient<TensorType>::AddExportGradient(GradientType &gradient)
{
  std::unique_lock<std::mutex> lock(export_buffer_cv_mutex_);
  {
    FETCH_LOCK(export_buffer_mutex_);
    export_buffer_.push(gradient);
  }
  export_buffer_cv_.notify_all();
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
  for (auto &trainable_lookup : g_ptr_->trainable_lookup_)
  {
    auto trainable_ptr =
        std::dynamic_pointer_cast<ops::Trainable<TensorType>>((trainable_lookup.second)->GetOp());
    trainable_ptr->SetWeights(*weights_it);
    ++weights_it;
  }
}

template <class TensorType>
void TrainingClient<TensorType>::GetNewGradients(VectorTensorType &new_gradients)
{
  FETCH_LOCK(queue_mutex_);
  new_gradients = gradient_queue_.front().first;
  gradient_queue_.pop();
}

// Timestamp for logging
template <class TensorType>
std::string TrainingClient<TensorType>::GetStrTimestamp()
{
  auto now       = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  auto now_milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%d-%H:%M:%S");

  // add milliseconds to timestamp string
  ss << '.' << std::setfill('0') << std::setw(3) << now_milliseconds.count();

  return ss.str();
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
 * Do one batch training, run model on test set and write loss to csv file
 */
template <class TensorType>
void TrainingClient<TensorType>::TrainOnce()
{
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::app);

  DoBatch();

  // Validate loss for logging purpose
  Test();

  // Save loss variation data
  // Upload to https://plot.ly/create/#/ for visualisation
  if (lossfile)
  {
    lossfile << GetStrTimestamp() << ", " << static_cast<double>(train_loss_) << ", "
             << static_cast<double>(test_loss_) << "\n";
  }

  if (print_loss_)
  {
    // Lock console
    FETCH_LOCK(*console_mutex_ptr_);
    std::cout << "Client " << id_ << "\tTraining loss: " << static_cast<double>(train_loss_)
              << "\tTest loss: " << static_cast<double>(test_loss_) << std::endl;
  }

  opti_ptr_->IncrementEpochCounter();
  opti_ptr_->UpdateLearningRate();

  if (lossfile)
  {
    lossfile << GetStrTimestamp() << ", "
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
 * Do batch training repeatedly while coordinator state is set to RUN
 */
template <class TensorType>
void TrainingClient<TensorType>::TrainWithCoordinator()
{
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::app);

  while (coordinator_ptr_->GetState() == CoordinatorState::RUN)
  {
    DoBatch();
    coordinator_ptr_->IncrementIterationsCounter();

    // Validate loss for logging purpose
    Test();

    // Save loss variation data
    // Upload to https://plot.ly/create/#/ for visualisation

    if (lossfile)
    {
      lossfile << GetStrTimestamp() << ", " << static_cast<double>(train_loss_) << ", "
               << static_cast<double>(test_loss_) << "\n";
    }

    if (print_loss_)
    {
      // Lock console
      FETCH_LOCK(*console_mutex_ptr_);
      std::cout << "Client " << id_ << "\tTraining loss: " << static_cast<double>(train_loss_)
                << "\tTest loss: " << static_cast<double>(test_loss_) << std::endl;
    }
  }

  opti_ptr_->IncrementEpochCounter();
  opti_ptr_->UpdateLearningRate();

  if (lossfile)
  {
    lossfile << GetStrTimestamp() << ", "
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
    peers_ = coordinator_ptr_->NextPeersList(id_);

    // Load own gradient
    GradientType current_gradient = std::make_pair(g_ptr_->GetGradients(), GetTimestamp());

    // Add gradient to export queue
    AddExportGradient(current_gradient);

    // Load own gradient
    VectorTensorType new_gradients;

    // Sum all gradient in queue
    while (!gradient_queue_.empty())
    {
      GetNewGradients(new_gradients);
      GraphAddGradients(g_ptr_, new_gradients);
    }
  }

  // Apply sum of all gradients from queue along with own gradient
  {
    FETCH_LOCK(model_mutex_);
    opti_ptr_->ApplyGradients(batch_size_);
    opti_ptr_->IncrementBatchCounters(batch_size_);
    opti_ptr_->UpdateLearningRate();
  }
  batch_counter_++;
}

template <class TensorType>
void TrainingClient<TensorType>::ExportBufferLoop()
{
  GradientType                 gradient;
  std::unique_lock<std::mutex> l(export_buffer_cv_mutex_);

  while (!export_stopped_)
  {
    export_buffer_cv_.wait(l);
    if (export_stopped_)
    {
      break;
    }

    if (!export_buffer_.empty())
    {
      FETCH_LOCK(export_buffer_mutex_);
      {
        gradient = export_buffer_.front();
        export_buffer_.pop();
      }

      // Give gradients to peers
      for (SizeType i{0}; i < peers_.size(); ++i)
      {
        peers_[i]->QueueAddGradient(gradient);
      }
    }
  }
}

template <class TensorType>
void TrainingClient<TensorType>::Initialise()
{
  // Start export buffer thread
  export_buffer_thread_ = std::make_shared<std::thread>(&TrainingClient::ExportBufferLoop, this);
}

///////////////////////
/// private methods ///
///////////////////////

/**
 * Adds gradient to own gradient queue
 * @param gradient
 */
template <class TensorType>
void TrainingClient<TensorType>::QueueAddGradient(GradientType &gradient)
{
  FETCH_LOCK(queue_mutex_);
  gradient_queue_.push(gradient);
}

/**
 * Adds a vector of Tensors to the gradient accumulation of all the trainable pointers in the graph.
 * @param gradient
 */
template <class TensorType>
void TrainingClient<TensorType>::GraphAddGradients(GraphPtrType            g_ptr,
                                                   VectorTensorType const &gradients)
{
  assert(gradients.size() == g_ptr->GetTrainables().size());
  auto grad_it = gradients.begin();
  for (auto &trainable : g_ptr->GetTrainables())
  {
    auto weights_ptr = std::dynamic_pointer_cast<ops::Weights<TensorType>>(trainable);
    weights_ptr->AddToGradient(*grad_it);
    ++grad_it;
  }
}

template <class TensorType>
std::shared_ptr<fetch::ml::Graph<TensorType>> TrainingClient<TensorType>::GetModel()
{
  return g_ptr_;
}

}  // namespace distributed_learning
}  // namespace ml
}  // namespace fetch
