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

#include "core/random.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#define NUMBER_OF_CLIENTS 10
#define NUMBER_OF_PEERS 3
#define NUMBER_OF_ITERATIONS 20
#define BATCH_SIZE 128
#define SYNCHRONIZATION_INTERVAL 3
#define LEARNING_RATE .001f
#define TEST_SET_RATIO 0.03f

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType         = float;
using TensorType       = fetch::math::Tensor<DataType>;
using TensorVectorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

enum class CoordinatorState
{
  RUN,
  STOP,
};

class Coordinator
{
public:
  CoordinatorState state = CoordinatorState::RUN;
};

class TrainingClient
{
public:
  TrainingClient(std::string const &images, std::string const &labels, std::string const &id)
    : dataloader_(images, labels)
    , id_(id)
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

  void SetCoordinator(std::shared_ptr<Coordinator> &coordinator_ptr)
  {
    coordinator_ptr_ = coordinator_ptr;
  }

  /**
   * Main loop that runs in thread
   */
  void MainLoop()
  {
    std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::app);
    DataType      loss;

    while (coordinator_ptr_->state == CoordinatorState::RUN)
    {
      // Shuffle the peers list to get new contact for next update
      fetch::random::Shuffle(gen_, peers_, peers_);

      // Train one batch to create own gradient
      Train();

      // Put own gradient to peers queues
      BroadcastGradients();

      // Load own gradient
      TensorVectorType gradients = g_.GetGradients();
      TensorVectorType new_gradients;

      // Sum all gradient in queue
      while (!gradient_queue.empty())
      {
        {
          std::lock_guard<std::mutex> l(queue_mutex_);
          new_gradients = gradient_queue.front();
          gradient_queue.pop();
        }

        for (SizeType j{0}; j < gradients.size(); j++)
        {
          fetch::math::Add(gradients.at(j), new_gradients.at(j), gradients.at(j));
        }
      }

      // Apply sum of all gradients from queue along with own gradient
      ApplyGradient(gradients);

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
   * Train one batch
   * @return training batch loss
   */
  DataType Train()
  {
    dataloader_.SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
    dataloader_.SetRandomMode(true);

    DataType loss        = 0;
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
  void Test(DataType &test_loss)
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
  TensorVectorType GetGradients() const
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    return g_.GetGradients();
  }

  /**
   * @return vector of weights that represents the model
   */
  TensorVectorType GetWeights() const
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    return g_.get_weights();
  }

  /**
   * Add pointers to other clients
   * @param clients
   */
  void AddPeers(std::vector<std::shared_ptr<TrainingClient>> const &clients)
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
  void BroadcastGradients()
  {
    // Load own gradient
    TensorVectorType current_gradient = g_.GetGradients();

    // Give gradients to peers
    for (unsigned int i(0); i < number_of_peers_; ++i)
    {
      peers_[i]->AddGradient(current_gradient);
    }
  }

  /**
   * Adds gradient to own gradient queue
   * @param gradient
   */
  void AddGradient(TensorVectorType gradient)
  {
    {
      std::lock_guard<std::mutex> l(queue_mutex_);
      gradient_queue.push(gradient);
    }
  }

  /**
   * Applies gradient multiplied by -LEARNING_RATE
   * @param gradients
   */
  void ApplyGradient(TensorVectorType gradients)
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
  void SetWeights(TensorVectorType &new_weights)
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    g_.set_weights(new_weights);
  }

private:
  // Client's own graph and mutex to protect it's weights
  fetch::ml::Graph<TensorType> g_;
  std::mutex                   model_mutex_;

  // Client's own dataloader
  fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType> dataloader_;

  // Connection to other nodes
  std::vector<std::shared_ptr<TrainingClient>> peers_;

  // Access to coordinator
  std::shared_ptr<Coordinator> coordinator_ptr_;

  // Gradient queue access and mutex to protect it
  std::queue<TensorVectorType> gradient_queue;
  std::mutex                   queue_mutex_;

  // random number generator for shuffling peers
  fetch::random::LaggedFibonacciGenerator<> gen_;

  // Learning hyperparameters
  SizeType batch_size_      = BATCH_SIZE;
  float    test_set_ratio_  = TEST_SET_RATIO;
  SizeType number_of_peers_ = NUMBER_OF_PEERS;
  DataType learning_rate_   = LEARNING_RATE;

  // Client id (identification name)
  std::string id_;

  // Timestamp for logging
  std::string GetTimeStamp()
  {
    // TODO: Implement timestamp
    return "TIMESTAMP";
  }
};

int main(int ac, char **av)
{
  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::shared_ptr<Coordinator> coordinator = std::make_shared<Coordinator>();

  std::cout << "FETCH Distributed MNIST Demo -- Synchronised" << std::endl;
  std::srand((unsigned int)std::time(nullptr));

  std::vector<std::shared_ptr<TrainingClient>> clients(NUMBER_OF_CLIENTS);
  for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Instanciate NUMBER_OF_CLIENTS clients
    clients[i] = std::make_shared<TrainingClient>(av[1], av[2], std::to_string(i));
  }

  for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Give every client the full list of other clients
    clients[i]->AddPeers(clients);

    // Give each client pointer to coordinator
    clients[i]->SetCoordinator(coordinator);
  }

  // Main loop
  for (unsigned int it(0); it < NUMBER_OF_ITERATIONS; ++it)
  {

    // Start all clients
    coordinator->state = CoordinatorState::RUN;
    std::cout << "================= ITERATION : " << it << " =================" << std::endl;
    std::list<std::thread> threads;
    for (auto &c : clients)
    {
      threads.emplace_back([&c] { c->MainLoop(); });
    }

    std::this_thread::sleep_for(std::chrono::seconds(SYNCHRONIZATION_INTERVAL));

    // Send stop signal to all clients
    coordinator->state = CoordinatorState::STOP;

    // Wait for everyone to finish
    for (auto &t : threads)
    {
      t.join();
    }

    // Synchronize weights by giving all clients average of all client's weights
    TensorVectorType new_weights = clients[0]->GetWeights();

    // Sum all wights
    for (unsigned int i(1); i < NUMBER_OF_CLIENTS; ++i)
    {
      TensorVectorType other_weights = clients[i]->GetWeights();

      for (SizeType j{0}; j < other_weights.size(); j++)
      {
        fetch::math::Add(new_weights.at(j), other_weights.at(j), new_weights.at(j));
      }
    }

    // Divide weights by number of clients to calculate the average
    for (SizeType j{0}; j < new_weights.size(); j++)
    {
      fetch::math::Divide(new_weights.at(j), static_cast<DataType>(NUMBER_OF_CLIENTS),
                          new_weights.at(j));
    }

    // Update models of all clients by average model
    for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
    {
      clients[i]->SetWeights(new_weights);
    }
  }

  return 0;
}
