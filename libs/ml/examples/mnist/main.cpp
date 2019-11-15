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
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/utilities/mnist_utilities.hpp"

#include "core/time/to_seconds.hpp"
#include <iostream>
#include <memory>
#include <string>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::SizeType;

using GraphType      = typename fetch::ml::Graph<TensorType>;
using OptimiserType  = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;
using DataLoaderType = typename fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>;

int main(int ac, char **av)
{
  DataType learning_rate{0.01f};
  // SizeType subset_size{100};
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

  std::string input = g->AddNode<PlaceHolder<TensorType>>("Input", {});
  std::string label = g->AddNode<PlaceHolder<TensorType>>("Label", {});

  std::string layer_1 = g->AddNode<FullyConnected<TensorType>>(
      "FC1", {input}, 28u * 28u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->AddNode<FullyConnected<TensorType>>(
      "FC2", {layer_1}, 10u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string output = g->AddNode<FullyConnected<TensorType>>(
      "FC3", {layer_2}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX);
  std::string error = g->AddNode<CrossEntropyLoss<TensorType>>("Error", {output, label});

  auto mnist_images = fetch::ml::utilities::read_mnist_images<TensorType>(av[1]);
  auto mnist_labels = fetch::ml::utilities::read_mnist_labels<TensorType>(av[2]);
  mnist_labels      = fetch::ml::utilities::convert_labels_to_onehot(mnist_labels);

  // Initialise dataloader
  DataLoaderType data_loader;
  data_loader.AddData({mnist_images}, mnist_labels);

  // Initialise Optimiser
  OptimiserType optimiser(g, {input}, label, error, learning_rate);

  // Training loop
  DataType    loss;
  std::string output_csv_file =
      "/home/emmasmith/Development/ledger-vm-misc/TFF_MNIST/fetch_results.txt";
  std::ofstream lossfile(output_csv_file, std::ofstream::out);
  auto          start_time = std::chrono::steady_clock::now();
  for (SizeType i{0}; i < epochs; i++)
  {
    loss = optimiser.Run(data_loader, batch_size);  //, subset_size);
    std::cout << "Loss: " << loss << std::endl;
    double seconds = fetch::ToSeconds(std::chrono::steady_clock::now() - start_time);
    lossfile << "Time: " << seconds << " Epoch: " << i << " Loss: " << loss << std::endl;
  }

  return 0;
}
