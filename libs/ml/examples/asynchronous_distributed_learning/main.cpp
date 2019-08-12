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
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// Runs in about 40 sec on a 2018 MBP
// Remember to disable debug using | grep -v INFO
#define NUMBER_OF_CLIENTS 10
#define NUMBER_OF_PEERS 3
#define NUMBER_OF_ITERATIONS 20
#define BATCH_SIZE 32
#define NUMBER_OF_BATCHES 10
#define MERGE_RATIO .5f
#define LEARNING_RATE .01f

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::SizeType;

class TrainingClient
{
public:
  TrainingClient(std::string const &images, std::string const &labels)
    : dataloader_(images, labels, true)
  {
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

  void Train(unsigned int numberOfBatches)
  {
    float                                          loss = 0;
    CrossEntropyLoss<TensorType>                   criterion;
    std::pair<TensorType, std::vector<TensorType>> input;
    for (unsigned int i(0); i < numberOfBatches; ++i)
    {
      loss = 0;
      for (unsigned int j(0); j < BATCH_SIZE; ++j)
      {
        // Random sampling ensures that for relatively few training steps the proportion of shared
        // training data is low
        input = dataloader_.GetNext();
        g_.SetInput("Input", input.second.at(0));
        g_.SetInput("Label", input.first);
        {
          std::lock_guard<std::mutex> l(m_);
          TensorType                  loss_tensor = g_.ForwardPropagate("Error");
          loss += *(loss_tensor.begin());
          g_.BackPropagateError("Error");
        }
      }
      losses_values_.push_back(loss);
      {
        // Updating the weights
        std::lock_guard<std::mutex> l(m_);
        g_.Step(LEARNING_RATE);
      }
    }
    UpdateWeights();
  }

  fetch::ml::StateDict<fetch::math::Tensor<float>> GetStateDict() const
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(m_));
    return g_.StateDict();
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

  void UpdateWeights()
  {
    std::list<fetch::ml::StateDict<TensorType>> stateDicts;
    for (unsigned int i(0); i < NUMBER_OF_PEERS; ++i)
    {
      // Collect the stateDicts from randomly selected peers
      stateDicts.push_back(peers_[i]->GetStateDict());
    }
    fetch::ml::StateDict<TensorType> averageStateDict =
        fetch::ml::StateDict<TensorType>::MergeList(stateDicts);
    {
      std::lock_guard<std::mutex> l(m_);
      g_.LoadStateDict(g_.StateDict().Merge(averageStateDict, MERGE_RATIO));
    }
    // Shuffle the peers list to get new contact for next update
    fetch::random::Shuffle(gen_, peers_, peers_);
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
  std::mutex m_;

  // random number generator for shuffling peers
  fetch::random::LaggedFibonacciGenerator<> gen_;
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
    std::cout << "================= ITERATION : " << it << " =================" << std::endl;
    std::list<std::thread> threads;
    for (auto &c : clients)
    {
      // Start each client to train on NUMBER_OF_BATCHES * BATCH_SIZE examples
      threads.emplace_back([&c] { c->Train(NUMBER_OF_BATCHES); });
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
