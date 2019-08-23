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
#include "math/statistics/mean.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// Runs in about 40 sec on a 2018 MBP
// Remember to disable debug using | grep -v INFO
#define NUMBER_OF_CLIENTS 10
#define NUMBER_OF_PEERS 3
#define NUMBER_OF_ITERATIONS 10
#define BATCH_SIZE 32
//#define NUMBER_OF_BATCHES 3
#define SYNCHRONIZATION_INTERVAL 1
#define MERGE_RATIO .5f
#define LEARNING_RATE .001f
#define TEST_SET_RATIO 0.03f

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType         = float;
using TensorType       = fetch::math::Tensor<DataType>;
using TensorVectorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

class TrainingClient
{
public:
  TrainingClient(std::string const &images, std::string const &labels)
    : dataloader_(images, labels)
  {
    dataloader_.SetTestRatio(TEST_SET_RATIO);
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
  }

  void MainLoop()
  {

    DataType loss;

    for (SizeType i{0}; i < 20; i++)
    {
      // Create own gradient
      Train();

      BroadcastGradients();

      // Process gradients que
      TensorVectorType new_gradients;

      // Load own gradient
      TensorVectorType gradients = g_.GetGradients();

      // Sum all gradient in que
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

      ApplyGradient(gradients);

      // Validate loss for logging purpose
      Test(loss);
      losses_values_.push_back(loss);

      // Shuffle the peers list to get new contact for next update
      fetch::random::Shuffle(gen_, peers_, peers_);
    }
  }

  DataType Train()
  {
    dataloader_.SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);

    DataType loss        = 0;
    bool     is_done_set = false;

    // Random sampling ensures that for relatively few training steps the proportion of shared
    // training data is low

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

  void Test(DataType &test_loss)
  {
    dataloader_.SetMode(fetch::ml::dataloaders::DataLoaderMode::TEST);
    dataloader_.SetRandomMode(false);

    SizeType test_set_size = dataloader_.Size();

    dataloader_.Reset();
    bool is_done_set;
    auto test_pair = dataloader_.PrepareBatch(test_set_size, is_done_set);
    dataloader_.SetRandomMode(true);
    {
      std::lock_guard<std::mutex> l(model_mutex_);

      g_.SetInput("Input", test_pair.second.at(0));
      g_.SetInput("Label", test_pair.first);

      test_loss = *(g_.ForwardPropagate("Error").begin());
    }
  }

  TensorVectorType GetGradients() const
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    return g_.GetGradients();
  }

  TensorVectorType GetWeights() const
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    return g_.get_weights();
  }

  void AddPeers(std::vector<std::shared_ptr<TrainingClient>> const &clients)
  {
    for (auto const &p : clients)
    {
      if (p.get() != this)
      {
        peers_.push_back(p);
      }
    }
    fetch::random::Shuffle(gen_, peers_, peers_);
  }

  void BroadcastGradients()
  {
    // Load own gradient
    TensorVectorType current_gradient = g_.GetGradients();

    // Give gradients to peers
    for (unsigned int i(0); i < NUMBER_OF_PEERS; ++i)
    {
      peers_[i]->AddGradient(current_gradient);
    }
  }

  void AddGradient(TensorVectorType gradient)
  {

    {
      std::lock_guard<std::mutex> l(queue_mutex_);
      gradient_queue.push(gradient);
    }
  }

  void ApplyGradient(TensorVectorType gradients)
  {
    // SGD
    for (SizeType j{0}; j < gradients.size(); j++)
    {
      fetch::math::Multiply(gradients.at(j), -LEARNING_RATE, gradients.at(j));
    }

    // Apply gradients
    {
      std::lock_guard<std::mutex> l(model_mutex_);
      g_.ApplyGradients(gradients);
    }
  }

  void SetWeights(TensorVectorType &new_weights)
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    g_.set_weights(new_weights);
  }

  std::vector<float> const &GetLossesValues() const
  {
    return losses_values_;
  }

private:
  // Client own graph
  fetch::ml::Graph<TensorType> g_;

  // Client own dataloader
  fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType> dataloader_;

  // Loss history
  std::vector<float> losses_values_;

  // Connection to other nodes
  std::vector<std::shared_ptr<TrainingClient>> peers_;

  // Mutex to protect weight access
  std::mutex model_mutex_;
  std::mutex queue_mutex_;

  // random number generator for shuffling peers
  fetch::random::LaggedFibonacciGenerator<> gen_;

  SizeType batch_size_ = BATCH_SIZE;

  std::queue<TensorVectorType> gradient_queue;
};

int main(int ac, char **av)
{
  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::cout << "FETCH Distributed MNIST Demo -- Synchronised" << std::endl;
  std::srand((unsigned int)std::time(nullptr));

  std::vector<std::shared_ptr<TrainingClient>> clients(NUMBER_OF_CLIENTS);
  for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Instanciate NUMBER_OF_CLIENTS clients
    clients[i] = std::make_shared<TrainingClient>(av[1], av[2]);
  }
  for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Give every client the full list of other clients
    clients[i]->AddPeers(clients);
  }

  for (unsigned int it(0); it < NUMBER_OF_ITERATIONS; ++it)
  {

    if (it % SYNCHRONIZATION_INTERVAL == 0)
    {
      TensorVectorType weights = clients[0]->GetWeights();
      for (unsigned int i(1); i < NUMBER_OF_CLIENTS; ++i)
      {
        clients[i]->SetWeights(weights);
      }
    }

    std::cout << "================= ITERATION : " << it << " =================" << std::endl;
    std::list<std::thread> threads;
    for (auto &c : clients)
    {
      // Start each client to train on NUMBER_OF_BATCHES * BATCH_SIZE examples
      threads.emplace_back([&c] { c->MainLoop(); });
    }
    for (auto &t : threads)
    {
      // Wait for everyone to finish (force synchronisation)
      t.join();
    }
  }

  // Save loss variation data
  // Upload to https://plot.ly/create/#/ for visualisation
  std::ofstream lossfile("losses.csv", std::ofstream::out | std::ofstream::trunc);
  if (lossfile)
  {
    for (unsigned int i(0); i < clients.size(); ++i)
    {
      lossfile << "Client " << i << ", ";
    }
    lossfile << "\n";
    for (unsigned int i(0); i < clients.front()->GetLossesValues().size(); ++i)
    {
      for (auto &c : clients)
      {
        lossfile << c->GetLossesValues()[i] << ", ";
      }
      lossfile << "\n";
    }
  }
  lossfile.close();

  return 0;
}
