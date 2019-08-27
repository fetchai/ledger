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

#include "../../examples/distributed_learning/distributed_learning_client.hpp"

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

TrainingClient::TrainingClient(std::string const &images, std::string const &labels,
                               std::string const &id, SizeType batch_size, DataType learning_rate,
                               float test_set_ratio, SizeType number_of_peers)
  : dataloader_(images, labels)
  , id_(std::move(id))
  , batch_size_(batch_size)
  , learning_rate_(learning_rate)
  , test_set_ratio_(test_set_ratio)
  , number_of_peers_(number_of_peers)
{

  dataloader_.SetTestRatio(test_set_ratio_);
  dataloader_.SetRandomMode(true);
  g_.AddNode<PlaceHolder<TensorType>>("Input", {});
  g_.AddNode<FullyConnected<TensorType>>("FC1", {"Input"}, 28u * 28u, 10u);
  g_.AddNode<Relu<TensorType>>("Relu1", {"FC1"});
  g_.AddNode<FullyConnected<TensorType>>("FC2", {"Relu1"}, 10u, 10u);
  g_.AddNode<Relu<TensorType>>("Relu2", {"FC1"});
  g_.AddNode<FullyConnected<TensorType>>("FC3", {"Relu2"}, 10u, 10u);
  g_.AddNode<Softmax<TensorType>>("Softmax", {"FC3"});
  g_.AddNode<PlaceHolder<TensorType>>("Label", {});
  g_.AddNode<CrossEntropyLoss<TensorType>>("Error", {"Softmax", "Label"});

  // Clear loss file
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::trunc);
  lossfile.close();
}

void TrainingClient::SetCoordinator(std::shared_ptr<Coordinator> coordinator_ptr)
{
  coordinator_ptr_ = coordinator_ptr;
}

/**
 * Main loop that runs in thread
 */
void TrainingClient::MainLoop()
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
DataType TrainingClient::Train()
{
  dataloader_.SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
  dataloader_.SetRandomMode(true);

  DataType loss        = static_cast<DataType>(0);
  bool     is_done_set = false;

  std::pair<TensorType, std::vector<TensorType>> input;
  input = dataloader_.PrepareBatch(batch_size_, is_done_set);

  {
    std::lock_guard<std::mutex> l(model_mutex_);

    g_.SetInput("Input", input.second.at(0));
    g_.SetInput("Label", input.first);

    TensorType loss_tensor = g_.ForwardPropagate("Error");
    loss                   = *(loss_tensor.begin());
    g_.BackPropagateError("Error");
  }

  return loss;
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
void TrainingClient::Test(DataType &test_loss)
{
  dataloader_.SetMode(fetch::ml::dataloaders::DataLoaderMode::TEST);

  // Disable random to run model on whole test set
  dataloader_.SetRandomMode(false);

  SizeType test_set_size = dataloader_.Size();

  dataloader_.Reset();
  bool is_done_set;
  auto test_pair = dataloader_.PrepareBatch(test_set_size, is_done_set);
  {
    std::lock_guard<std::mutex> l(model_mutex_);

    g_.SetInput("Input", test_pair.second.at(0));
    g_.SetInput("Label", test_pair.first);

    test_loss = *(g_.Evaluate("Error").begin());
  }
}

/**
 * @return vector of gradient update values
 */
VectorTensorType TrainingClient::GetGradients() const
{
  std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
  return g_.GetGradients();
}

/**
 * @return vector of weights that represents the model
 */
VectorTensorType TrainingClient::GetWeights() const
{
  std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
  return g_.get_weights();
}

/**
 * Add pointers to other clients
 * @param clients
 */
void TrainingClient::AddPeers(std::vector<std::shared_ptr<TrainingClient>> const &clients)
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
void TrainingClient::BroadcastGradients()
{
  // Load own gradient
  VectorTensorType current_gradient = g_.GetGradients();

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
void TrainingClient::AddGradient(VectorTensorType gradient)
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
void TrainingClient::ApplyGradient(VectorTensorType gradients)
{
  // Step of SGD optimiser
  for (SizeType j{0}; j < gradients.size(); j++)
  {
    fetch::math::Multiply(gradients.at(j), -learning_rate_, gradients.at(j));
  }

  // Apply gradients to own model
  {
    std::lock_guard<std::mutex> l(model_mutex_);
    g_.ApplyGradients(gradients);
  }
}

/**
 * Rewrites current model with given one
 * @param new_weights Vector of weights that represent model
 */
void TrainingClient::SetWeights(VectorTensorType &new_weights)
{
  std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
  g_.SetWeights(new_weights);
}

void TrainingClient::GetNewGradients(VectorTensorType &new_gradients)
{
  std::lock_guard<std::mutex> l(queue_mutex_);
  new_gradients = gradient_queue_.front();
  gradient_queue_.pop();
}

// Timestamp for logging
std::string TrainingClient::GetTimeStamp()
{
  // TODO(1564): Implement timestamp
  return "TIMESTAMP";
}

/**
 * Do one batch training, run model on test set and write loss to csv file
 */
void TrainingClient::TrainOnce()
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
void TrainingClient::TrainWithCoordinator()
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
void TrainingClient::DoBatch()
{
  // Train one batch to create own gradient
  Train();

  VectorTensorType gradients = g_.GetGradientsReferences();

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

      for (SizeType j{0}; j < gradients.size(); j++)
      {
        fetch::math::Add(gradients.at(j), new_gradients.at(j), gradients.at(j));
      }
    }
  }

  // Apply sum of all gradients from queue along with own gradient
  ApplyGradient(gradients);
}
