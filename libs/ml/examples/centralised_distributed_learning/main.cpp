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

#include "math/statistics/mean.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// Runs in about 40 sec on a 2018 MBP
// Remember to disable debug using | grep -v INFO
#define NUMBER_OF_CLIENTS 10
#define NUMBER_OF_ITERATIONS 20
#define BATCH_SIZE 32
#define NUMBER_OF_BATCHES 10
#define LEARNING_RATE 0.01f

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;

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
    g_.AddNode<Relu<TensorType>>("Relu2", {"FC2"});
    g_.AddNode<FullyConnected<TensorType>>("FC3", {"Relu2"}, 10u, 10u);
    g_.AddNode<Softmax<TensorType>>("Softmax", {"FC3"});
    g_.AddNode<PlaceHolder<TensorType>>("Label", {});
    g_.AddNode<CrossEntropyLoss<TensorType>>("Error", {"Softmax", "Label"});
  }

  void Train(unsigned int number_of_batches)
  {
    float                                          loss = 0;
    CrossEntropyLoss<TensorType>                   criterion;
    std::pair<TensorType, std::vector<TensorType>> input;
    for (unsigned int i(0); i < number_of_batches; ++i)
    {
      loss = 0;
      for (unsigned int j(0); j < BATCH_SIZE; ++j)
      {
        // Random sampling ensures that for relatively few training steps the proportion of shared
        // training data is low
        input = dataloader_.GetNext();
        g_.SetInput("Input", input.second.at(0));
        g_.SetInput("Label", input.first);

        TensorType loss_tensor = g_.ForwardPropagate("Error");
        loss += *(loss_tensor.begin());
        g_.BackPropagateError("Error");
      }
      losses_values_.push_back(loss);

      // Updating the weights
      for (auto &w : g_.GetTrainables())
      {
        TensorType grad = w->get_gradients();
        fetch::math::Multiply(grad, DataType{-LEARNING_RATE}, grad);
        w->ApplyGradient(grad);
      }
    }
  }

  fetch::ml::StateDict<fetch::math::Tensor<float>> GetStateDict() const
  {
    return g_.StateDict();
  }

  void LoadStateDict(fetch::ml::StateDict<fetch::math::Tensor<float>> const &sd)
  {
    g_.LoadStateDict(sd);
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
};

int main(int ac, char **av)
{
  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::cout << "FETCH Distributed (with central controller) MNIST Demo" << std::endl;

  std::list<TrainingClient> clients;
  for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    clients.emplace_back(av[1], av[2]);
  }

  for (unsigned int it(0); it < NUMBER_OF_ITERATIONS; ++it)
  {
    std::cout << "================= ITERATION : " << it << " =================" << std::endl;
    std::list<std::thread> threads;
    for (auto &c : clients)
    {
      // Start each client to train on NUMBER_OF_BATCHES * BATCH_SIZE examples
      threads.emplace_back([&c] { c.Train(NUMBER_OF_BATCHES); });
    }
    for (auto &t : threads)
    {
      // Wait for everyone to be done
      t.join();
    }
    std::list<fetch::ml::StateDict<TensorType>> stateDicts;
    for (auto &c : clients)
    {
      // Collect all the stateDicts
      stateDicts.push_back(c.GetStateDict());
    }
    // Average them together
    fetch::ml::StateDict<TensorType> averageStateDict =
        fetch::ml::StateDict<TensorType>::MergeList(stateDicts);
    for (auto &c : clients)
    {
      // Load newly averaged weight into each client
      c.LoadStateDict(averageStateDict);
    }

    // Do some evaluation here
  }

  // Save loss variation data
  // Upload to https://plot.ly/create/#/ for visualisation
  std::ofstream myfile("losses.csv", std::ofstream::out | std::ofstream::trunc);
  if (myfile)
  {
    for (unsigned int i(0); i < clients.size(); ++i)
    {
      myfile << "Client " << i << ", ";
    }
    myfile << "\n";
    for (unsigned int i(0); i < clients.front().GetLossesValues().size(); ++i)
    {
      for (auto &c : clients)
      {
        myfile << c.GetLossesValues()[i] << ", ";
      }
      myfile << "\n";
    }
  }
  myfile.close();

  return 0;
}
