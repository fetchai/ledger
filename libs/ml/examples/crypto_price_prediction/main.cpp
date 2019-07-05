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
//#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/mean_square_error.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
//#include "ml/regularisers/l1_regulariser.hpp"
//#include "ml/regularisers/regularisation.hpp"
#include "ml/dataloaders/ReadCSV.hpp"

//#include <cstddef>
//#include <cstdint>
//#include <iostream>
//#include <string>
//#include <unordered_map>
//#include <utility>
//#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

using GraphType        = typename fetch::ml::Graph<ArrayType>;
using CostFunctionType = typename fetch::ml::ops::MeanSquareError<ArrayType>;
using OptimiserType    = typename fetch::ml::optimisers::AdamOptimiser<ArrayType, CostFunctionType>;
// using DataLoaderType   = typename fetch::ml::dataloaders::MNISTLoader<ArrayType, ArrayType>;

std::shared_ptr<GraphType> BuildModel(std::string &input_name, std::string &output_name)
{
  auto g = std::make_shared<GraphType>();

  SizeType conv1D_1_filters        = 8;
  SizeType conv1D_1_input_channels = 1;
  SizeType conv1D_1_kernel_size    = 20;
  SizeType conv1D_1_stride         = 3;

  typename ArrayType::Type keep_prob = 0.5;

  SizeType conv1D_2_filters        = 1;
  SizeType conv1D_2_input_channels = 1;
  SizeType conv1D_2_kernel_size    = 16;
  SizeType conv1D_2_stride         = 4;

  input_name          = g->AddNode<PlaceHolder<ArrayType>>("Input", {});
  std::string layer_1 = g->AddNode<fetch::ml::layers::Convolution1D<ArrayType>>(
      "Conv1D_1", {input_name}, conv1D_1_filters, conv1D_1_input_channels, conv1D_1_kernel_size,
      conv1D_1_stride, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->AddNode<Dropout<ArrayType>>("Dropout_1", {layer_1}, keep_prob);
  output_name         = g->AddNode<fetch::ml::layers::Convolution1D<ArrayType>>(
      "Conv1D_2", {layer_2}, conv1D_2_filters, conv1D_2_input_channels, conv1D_2_kernel_size,
      conv1D_2_stride);

  return g;
}

// TODO: convert to a dataloader
std::vector<ArrayType> LoadData(std::string const &data_filename, std::string const &labels_filename)
{
  auto data_tensor = fetch::ml::dataloaders::ReadCSV<ArrayType>(data_filename);
  auto labels_tensor = fetch::ml::dataloaders::ReadCSV<ArrayType>(labels_filename);
  return {data_tensor, labels_tensor};
}

int main(int ac, char **av)
{
  //  SizeType                               subset_size{100};
  SizeType epochs{10};
  SizeType batch_size{8};
  //  fetch::ml::details::RegularisationType regulariser =
  //  fetch::ml::details::RegularisationType::L1; DataType reg_rate{0.01f};

  if (ac < 3)
  {
    std::cout << "Usage : " << av[0] << " PATH/TO/train-prices PATH/TO/test-prices" << std::endl;
    return 1;
  }

  std::cout << "FETCH Crypto price prediction demo" << std::endl;

  // Load training data
  std::vector<ArrayType> data_and_labels = LoadData(av[1], av[2]);
  //  DataLoaderType data_loader(av[1], av[2]);

  // Prepare graph & optimiser
  std::string input_name  = "";
  std::string output_name = "";
  std::shared_ptr<GraphType> g = BuildModel(input_name, output_name);
  OptimiserType              optimiser(g, {input_name}, output_name);

  // Training loop
  DataType loss;
  for (SizeType i{0}; i < epochs; i++)
  {
    loss = optimiser.Run({data_and_labels.at(0)}, data_and_labels.at(1), batch_size);
    std::cout << "Loss: " << loss << std::endl;
  }

  return 0;
}
