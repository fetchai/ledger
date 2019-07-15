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

// TODO: try out linear decay weightings on keras conv models
// TODO: MSE now works with weightings, so set up exponential decay weightings in training
// TODO: implement normalisation in etch crypto prediction
// TODO: implement weightin in etch crypto prediction

#include "math/metrics/mean_absolute_error.hpp"
#include "math/normalize_array.hpp"
#include "math/tensor.hpp"

#include "vectorise/fixed_point/fixed_point.hpp"

#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/utilities/min_max_scaler.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = fetch::fixed_point::fp64_t;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = typename TensorType::SizeType;

using GraphType        = typename fetch::ml::Graph<TensorType>;
using CostFunctionType = typename fetch::ml::ops::MeanSquareErrorLoss<TensorType>;
using OptimiserType    = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;
using DataLoaderType   = typename fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>;

struct TrainingParams
{
  SizeType epochs{10};
  SizeType batch_size{1000};
  bool     normalise = true;
};

std::shared_ptr<GraphType> BuildModel(std::string &input_name, std::string &output_name,
                                      std::string &label_name, std::string &error_name)
{
  auto g = std::make_shared<GraphType>();

  SizeType conv1D_1_filters        = 16;
  SizeType conv1D_1_input_channels = 1;
  SizeType conv1D_1_kernel_size    = 96;
  SizeType conv1D_1_stride         = 3;

  typename TensorType::Type keep_prob_1{0.9};

  SizeType conv1D_2_filters        = 8;
  SizeType conv1D_2_input_channels = conv1D_1_filters;
  SizeType conv1D_2_kernel_size    = 48;
  SizeType conv1D_2_stride         = 2;

  typename TensorType::Type keep_prob_2{0.9};

  SizeType conv1D_3_filters        = 1;
  SizeType conv1D_3_input_channels = conv1D_2_filters;
  SizeType conv1D_3_kernel_size    = 47;
  SizeType conv1D_3_stride         = 1;

  input_name = g->AddNode<PlaceHolder<TensorType>>("Input", {});
  label_name = g->AddNode<PlaceHolder<TensorType>>("Label", {});

  std::string layer_1 = g->AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      "Conv1D_1", {input_name}, conv1D_1_filters, conv1D_1_input_channels, conv1D_1_kernel_size,
      conv1D_1_stride, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->AddNode<Dropout<TensorType>>("Dropout_1", {layer_1}, keep_prob_1);

  std::string layer_3 = g->AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      "Conv1D_2", {layer_2}, conv1D_2_filters, conv1D_2_input_channels, conv1D_2_kernel_size,
      conv1D_2_stride, fetch::ml::details::ActivationType::RELU);
  std::string layer_4 = g->AddNode<Dropout<TensorType>>("Dropout_2", {layer_3}, keep_prob_2);

  output_name = g->AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      "Conv1D_3", {layer_4}, conv1D_3_filters, conv1D_3_input_channels, conv1D_3_kernel_size,
      conv1D_3_stride);

  error_name = g->AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
      "error_name", {output_name, label_name});
  return g;
}

std::vector<TensorType> LoadData(std::string const &train_data_filename,
                                 std::string const &train_labels_filename,
                                 std::string const &test_data_filename,
                                 std::string const &test_labels_filename)
{

  std::cout << "loading train data...: " << std::endl;
  auto train_data_tensor =
      fetch::ml::dataloaders::ReadCSV<TensorType>(train_data_filename, 0, 0, true);

  std::cout << "loading train labels...: " << std::endl;
  auto train_labels_tensor =
      fetch::ml::dataloaders::ReadCSV<TensorType>(train_labels_filename, 0, 0, true);

  std::cout << "loading test data...: " << std::endl;
  auto test_data_tensor =
      fetch::ml::dataloaders::ReadCSV<TensorType>(test_data_filename, 0, 0, true);

  std::cout << "loading test labels...: " << std::endl;
  auto test_labels_tensor =
      fetch::ml::dataloaders::ReadCSV<TensorType>(test_labels_filename, 0, 0, true);

  train_data_tensor.Reshape({1, train_data_tensor.shape().at(0), train_data_tensor.shape().at(1)});
  train_labels_tensor.Reshape(
      {1, train_labels_tensor.shape().at(0), train_labels_tensor.shape().at(1)});
  test_data_tensor.Reshape({1, test_data_tensor.shape().at(0), test_data_tensor.shape().at(1)});
  test_labels_tensor.Reshape(
      {1, test_labels_tensor.shape().at(0), test_labels_tensor.shape().at(1)});
  return {train_data_tensor, train_labels_tensor, test_data_tensor, test_labels_tensor};
}

int main(int ac, char **av)
{
  if (ac < 5)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-data PATH/TO/train-labels PATH/TO/test-data PATH/TO/test-labels"
              << std::endl;
    return 1;
  }

  TrainingParams                                 tp;
  fetch::ml::utilities::MinMaxScaler<TensorType> scaler;

  std::cout << "FETCH Crypto price prediction demo" << std::endl;

  std::cout << "Loading crypto price data... " << std::endl;
  std::vector<TensorType> data_and_labels  = LoadData(av[1], av[2], av[3], av[4]);
  TensorType              orig_train_data  = data_and_labels.at(0);
  TensorType              orig_train_label = data_and_labels.at(1);
  TensorType              orig_test_data   = data_and_labels.at(2);
  TensorType              orig_test_label  = data_and_labels.at(3);
  TensorType              train_data(orig_train_data);
  TensorType              train_label(orig_train_label);
  TensorType              test_data(orig_test_data);
  TensorType              test_label(orig_test_label);
  test_label.Reshape({test_label.shape().at(1), test_label.shape().at(2)});

  if (tp.normalise)
  {
    scaler.SetScale(train_data);

    // normalise training and test data with respect to training data range
    scaler.Normalise(orig_train_data, train_data);
    scaler.Normalise(orig_train_label, train_label);
    scaler.Normalise(orig_test_data, test_data);
    scaler.Normalise(orig_test_label, test_label);
  }

  DataLoaderType loader(train_label.shape(), {train_data.shape()}, false);
  loader.AddData(train_data, train_label);

  std::cout << "Build model & optimiser... " << std::endl;
  std::string                input_name, output_name, label_name, error_name;
  std::shared_ptr<GraphType> g = BuildModel(input_name, output_name, label_name, error_name);

  fetch::ml::optimisers::LearningRateParam<DataType> learning_rate_param;
  learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::LINEAR;
  OptimiserType optimiser(g, {input_name}, label_name, error_name, learning_rate_param);

  std::cout << "Begin training loop... " << std::endl;
  for (SizeType i{0}; i < tp.epochs; i++)
  {
    optimiser.Run(loader, tp.batch_size);

    g->SetInput(input_name, test_data);
    auto prediction = g->Evaluate(output_name, false);
    prediction.Reshape({prediction.shape().at(1), prediction.shape().at(2)});

    if (tp.normalise)
    {
      scaler.DeNormalise(prediction, prediction);
    }
    auto result = fetch::math::MeanAbsoluteError(prediction, orig_test_label);
    std::cout << "mean absolute validation error: " << result << std::endl;
  }

  return 0;
}
