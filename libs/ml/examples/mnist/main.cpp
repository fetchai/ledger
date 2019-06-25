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

using GraphType        = typename fetch::ml::Graph<ArrayType>;
using CostFunctionType = typename fetch::ml::ops::CrossEntropy<ArrayType>;
using OptimiserType    = typename fetch::ml::optimisers::AdamOptimiser<ArrayType, CostFunctionType>;
using DataLoaderType   = typename fetch::ml::dataloaders::MNISTLoader<ArrayType, ArrayType>;

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
  auto g = std::make_shared<GraphType>();

  std::string input   = g->AddNode<PlaceHolder<ArrayType>>("Input", {});
  std::string layer_1 = g->AddNode<FullyConnected<ArrayType>>(
      "FC1", {input}, 28u * 28u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->AddNode<FullyConnected<ArrayType>>(
      "FC2", {layer_1}, 10u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string output = g->AddNode<FullyConnected<ArrayType>>(
      "FC3", {layer_2}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX);

  // Initialise MNIST loader
  DataLoaderType data_loader(av[1], av[2]);

  // Initialise Optimiser
  OptimiserType optimiser(g, {input}, output, learning_rate);

  // Training loop
  DataType loss;
  for (SizeType i{0}; i < epochs; i++)
  {
    loss = optimiser.Run(data_loader, batch_size, subset_size);
    std::cout << "Loss: " << loss << std::endl;
  }

  return 0;
}
