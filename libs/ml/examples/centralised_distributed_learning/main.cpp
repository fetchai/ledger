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

#include "math/tensor.hpp"
#include "ml/dataloaders/mnist_loader.hpp"
#include "ml/graph.hpp"

#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"

#include <fstream>
#include <iostream>
#include <thread>

// Runs in about 40 sec on a 2018 MBP
// Remember to disable debug using | grep -v INFO
#define NUMBER_OF_CLIENTS 10
#define NUMBER_OF_ITERATIONS 20
#define BATCH_SIZE 32
#define NUMBER_OF_BATCHES 10
#define LEARNING_RATE 0.01f

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;

class TrainingClient
{
public:
  TrainingClient(std::string const &images, std::string const &labels)
    : dataloader_(images, labels)
  {
    g_.AddNode<PlaceHolder<ArrayType>>("Input", {});
    g_.AddNode<FullyConnected<ArrayType>>("FC1", {"Input"}, 28u * 28u, 10u);
    g_.AddNode<Relu<ArrayType>>("Relu1", {"FC1"});
    g_.AddNode<FullyConnected<ArrayType>>("FC2", {"Relu1"}, 10u, 10u);
    g_.AddNode<Relu<ArrayType>>("Relu2", {"FC1"});
    g_.AddNode<FullyConnected<ArrayType>>("FC3", {"Relu2"}, 10u, 10u);
    g_.AddNode<Softmax<ArrayType>>("Softmax", {"FC3"});
  }

  void Train(unsigned int number_of_batches)
  {
    float                             loss = 0;
    CrossEntropy<ArrayType>           criterion;
    std::pair<std::size_t, ArrayType> input;
    ArrayType                         gt(std::vector<typename ArrayType::SizeType>({1, 10}));
    for (unsigned int i(0); i < number_of_batches; ++i)
    {
      loss = 0;
      for (unsigned int j(0); j < BATCH_SIZE; ++j)
      {
        // Random sampling ensures that for relatively few training steps the proportion of shared
        // training data is low
        input = dataloader_.GetRandom();
        g_.SetInput("Input", input.second);
        gt.Fill(0);
        gt.At(input.first)       = DataType(1.0);
        ArrayType const &results = g_.Evaluate("Softmax");
        loss += criterion.Forward({results, gt});
        g_.BackPropagate("Softmax", criterion.Backward({results, gt}));
      }
      losses_values_.push_back(loss);
      // Updating the weights
      g_.Step(LEARNING_RATE);
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
  fetch::ml::Graph<ArrayType> g_;
  // Client own dataloader
  fetch::ml::MNISTLoader<ArrayType> dataloader_;
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
    std::list<fetch::ml::StateDict<ArrayType>> stateDicts;
    for (auto &c : clients)
    {
      // Collect all the stateDicts
      stateDicts.push_back(c.GetStateDict());
    }
    // Average them together
    fetch::ml::StateDict<ArrayType> averageStateDict =
        fetch::ml::StateDict<ArrayType>::MergeList(stateDicts);
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
