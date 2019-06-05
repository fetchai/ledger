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
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

int main(int ac, char **av)
{
  DataType learning_rate{0.01f};
  SizeType subset_size{100};
  SizeType epochs{10};
  SizeType batch_size{10};

  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::cout << "FETCH MNIST Demo" << std::endl;

  // Prepare graph
  //  Input -> FC -> Relu -> FC -> Relu -> FC -> Softmax
  std::shared_ptr<fetch::ml::Graph<ArrayType>> g(std::make_shared<fetch::ml::Graph<ArrayType>>());

  std::string input_name = g->AddNode<PlaceHolder<ArrayType>>("Input", {});
  g->AddNode<FullyConnected<ArrayType>>("FC1", {"Input"}, 28u * 28u, 10u);
  g->AddNode<Relu<ArrayType>>("Relu1", {"FC1"});
  g->AddNode<FullyConnected<ArrayType>>("FC2", {"Relu1"}, 10u, 10u);
  g->AddNode<Relu<ArrayType>>("Relu2", {"FC1"});
  g->AddNode<FullyConnected<ArrayType>>("FC3", {"Relu2"}, 10u, 10u);
  std::string output_name = g->AddNode<Softmax<ArrayType>>("Softmax", {"FC3"});

  // Initialise MNIST loader
  std::shared_ptr<fetch::ml::DataLoader<ArrayType, ArrayType>> data_loader(
      std::make_shared<fetch::ml::MNISTLoader<ArrayType, ArrayType>>(av[1], av[2]));

  // Initialise Optimiser
  fetch::ml::optimisers::AdamOptimiser<ArrayType, fetch::ml::ops::CrossEntropy<ArrayType>>
      optimiser(g, input_name, output_name, learning_rate);

  // Training loop
  DataType loss;
  for (SizeType i{0}; i < epochs; i++)
  {
    loss = optimiser.Run(*data_loader, batch_size, subset_size);
    std::cout << "Loss: " << loss << std::endl;
  }

  return 0;
}
