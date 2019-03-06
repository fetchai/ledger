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
#include "ml/ops/cross_entropy.hpp"

#include <iostream>
#include <thread>

#define NUMBER_OF_CLIENTS 100
#define NUMBER_OF_ITERATIONS 1000
#define BATCHSIZE 32
#define NUMBER_OF_BATCHES 10

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

  float Train(unsigned int numberOfBatches)
  {
    float                                         loss = 0;
    CrossEntropyLayer<ArrayType>                  criterion;
    std::pair<size_t, std::shared_ptr<ArrayType>> input;
    std::shared_ptr<ArrayType>                    gt =
        std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({1, 10}));
    for (unsigned int i(0); i < numberOfBatches; ++i)
    {
      for (unsigned int j(0); j < BATCHSIZE; ++j)
      {
        // Randomly sampling through the dataset, should ensure everyone is training on different
        // data
        input = dataloader_.GetRandom();
        g_.SetInput("Input", input.second);
        gt->Fill(0);
        gt->At(input.first)                = DataType(1.0);
        std::shared_ptr<ArrayType> results = g_.Evaluate("Softmax");
        loss += criterion.Forward({results, gt});
        g_.BackPropagate("Softmax", criterion.Backward({results, gt}));
      }
      // Updating the weights
      g_.Step(0.01f);
    }
    return loss;
  }

  fetch::ml::StateDict<fetch::math::Tensor<float>> GetStateDict() const
  {
    return g_.StateDict();
  }

  void LoadStateDict(fetch::ml::StateDict<fetch::math::Tensor<float>> const &sd)
  {
    g_.LoadStateDict(sd);
  }

private:
  // Client own graph
  fetch::ml::Graph<ArrayType> g_;
  // Client own dataloader
  fetch::ml::MNISTLoader<ArrayType> dataloader_;
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
    std::list<std::thread> threads;
    for (auto &c : clients)
    {
      // Start each client to train on 100 examples
      threads.emplace_back([&c] { c.Train(NUMBER_OF_BATCHES); });
    }
    for (auto &t : threads)
    {
      // Wait for everyone to be done
      t.join();
    }
    std::list<const fetch::ml::StateDict<ArrayType>> stateDicts;
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

  return 0;
}
