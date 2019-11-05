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
#include "ml/core/graph.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/regularisers/l1_regulariser.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/utilities/mnist_utilities.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType = fetch::math::SizeType;

using GraphType      = typename fetch::ml::Graph<TensorType>;
using OptimiserType  = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;
using DataLoaderType = typename fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>;

int main(int ac, char **av)
{
  DataType                      learning_rate{0.01f};
  SizeType                      subset_size{100};
  SizeType                      epochs{10};
  SizeType                      batch_size{13};
  fetch::ml::RegularisationType regulariser = fetch::ml::RegularisationType::L1;
  DataType                      reg_rate{0.00f};

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

  std::string input = g->AddNode<PlaceHolder<TensorType>>("Input", {});
  std::string label = g->AddNode<PlaceHolder<TensorType>>("Label", {});
  std::string label_onehot = g->AddNode<OneHot<TensorType>>("Label_onehot", {label}, 10, 0);
  std::string label_flatten = g->AddNode<Flatten<TensorType>>("Label flatten", {label_onehot});

  std::string layer_1 = g->AddNode<FullyConnected<TensorType>>(
      "FC1", {input}, 28u * 28u, 10u, fetch::ml::details::ActivationType::RELU, regulariser,
      reg_rate);
  std::string layer_2 = g->AddNode<FullyConnected<TensorType>>(
      "FC2", {layer_1}, 10u, 10u, fetch::ml::details::ActivationType::RELU, regulariser, reg_rate);
  std::string output = g->AddNode<FullyConnected<TensorType>>(
      "FC3", {layer_2}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX, regulariser,
      reg_rate);
  std::string error = g->AddNode<CrossEntropyLoss<TensorType>>("Error", {output, label_flatten});

  auto     mnist_images = fetch::ml::utilities::ReadMnistImages<TensorType>(av[1]);
  auto mnist_labels = fetch::ml::utilities::ReadMnistLabels<TensorType>(av[2]);

  // Initialise MNIST loader
  fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType> data_loader;
  data_loader.AddData({mnist_images}, mnist_labels);

  // Initialise Optimiser
  OptimiserType optimiser(g, {input}, label, error, learning_rate);

  // Training loop
  DataType loss;
  for (SizeType i{0}; i < epochs; i++)
  {
    loss = optimiser.Run(data_loader, batch_size, subset_size);
    std::cout << "Loss: " << loss << std::endl;
  }

  return 0;
}
