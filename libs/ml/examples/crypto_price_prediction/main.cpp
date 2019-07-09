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

#include "math/normalize_array.hpp"
#include "math/tensor.hpp"

#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = typename TensorType::SizeType;

using GraphType        = typename fetch::ml::Graph<TensorType>;
using CostFunctionType = typename fetch::ml::ops::MeanSquareErrorLoss<TensorType>;
using OptimiserType    = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;
using DataLoaderType   = typename fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>;

std::shared_ptr<GraphType> BuildModel(std::string &input_name, std::string &output_name,
                                      std::string &label_name, std::string &error_name)
{
  auto g = std::make_shared<GraphType>();

  SizeType conv1D_1_filters        = 8;
  SizeType conv1D_1_input_channels = 1;
  SizeType conv1D_1_kernel_size    = 20;
  SizeType conv1D_1_stride         = 3;

  typename TensorType::Type keep_prob = 0.5;

  SizeType conv1D_2_filters        = 1;
  SizeType conv1D_2_input_channels = conv1D_1_filters;
  SizeType conv1D_2_kernel_size    = 16;
  SizeType conv1D_2_stride         = 4;

  input_name          = g->AddNode<PlaceHolder<TensorType>>("Input", {});
  label_name          = g->AddNode<PlaceHolder<TensorType>>("Label", {});
  std::string layer_1 = g->AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      "Conv1D_1", {input_name}, conv1D_1_filters, conv1D_1_input_channels, conv1D_1_kernel_size,
      conv1D_1_stride, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->AddNode<Dropout<TensorType>>("Dropout_1", {layer_1}, keep_prob);
  output_name         = g->AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      "Conv1D_2", {layer_2}, conv1D_2_filters, conv1D_2_input_channels, conv1D_2_kernel_size,
      conv1D_2_stride);

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

/**
 * MinMax scaler normaliser
 * @param input_data
 * @param reference_data
 * @return
 */
TensorType NormaliseFeatures(TensorType &input_data, TensorType const &reference_data)
{
  // set up return tensor
  TensorType ret(input_data.shape());

  // set up min, max, and range tensors
  TensorType x_min({1, reference_data.shape(1)});
  TensorType x_max({1, reference_data.shape(1)});
  TensorType x_range({1, reference_data.shape(1)});

  // calculate min, max, and range of each feature
  for (std::size_t i = 0; i < reference_data.shape(2); ++i)
  {
    auto x_min_it   = x_min.begin();
    auto x_max_it   = x_max.begin();
    auto x_range_it = x_range.begin();
    auto train_it   = input_data.Slice(i, 2).begin();
    while (x_min_it.is_valid())
    {
      if (*x_min_it > *train_it)
      {
        *x_min_it = *train_it;
      }

      if (*x_max_it < *train_it)
      {
        *x_max_it = *train_it;
      }

      if (*x_range_it < (*x_max_it - *x_min_it))
      {
        *x_range_it = (*x_max_it - *x_min_it);
      }

      ++train_it;
      ++x_min_it;
      ++x_max_it;
      ++x_range_it;
    }
  }

  // apply normalisation to each feature according to scale -1, 1
  for (std::size_t i = 0; i < reference_data.shape(2); ++i)
  {
    auto x_min_it   = x_min.begin();
    auto x_range_it = x_range.begin();
    auto ret_it     = ret.Slice(i, 2).begin();
    while (ret_it.is_valid())
    {
      *ret_it = (*ret_it - *x_min_it) / (*x_range_it);

      ++ret_it;
      ++x_min_it;
      ++x_range_it;
    }
  }
  return ret;
}
//
// void DeNormalisePrediction()
//{}

int main(int ac, char **av)
{
  SizeType epochs{10};
  SizeType batch_size{8};
  bool     normalise = false;

  if (ac < 5)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-data PATH/TO/train-labels PATH/TO/test-data PATH/TO/test-labels"
              << std::endl;
    return 1;
  }

  std::cout << "FETCH Crypto price prediction demo" << std::endl;

  std::cout << "Loading crypto price data... " << std::endl;
  std::vector<TensorType> data_and_labels = LoadData(av[1], av[2], av[3], av[4]);
  TensorType              train_data      = data_and_labels.at(0);
  TensorType              train_label     = data_and_labels.at(1);
  TensorType              test_data       = data_and_labels.at(2);
  TensorType              test_label      = data_and_labels.at(3);

  test_label.Reshape({test_label.shape().at(1), test_label.shape().at(2)});

  if (normalise)
  {
    TensorType orig_train_data  = train_data.Copy();
    TensorType orig_train_label = train_label.Copy();
    TensorType orig_test_data   = test_data.Copy();
    TensorType orig_test_label  = test_label.Copy();

    // normalise training and test data with respect to training data range
    train_data  = NormaliseFeatures(train_data, train_data);
    train_label = NormaliseFeatures(train_label, train_data);
    test_data   = NormaliseFeatures(test_data, train_data);
    test_label  = NormaliseFeatures(test_label, train_data);
  }

  //  DataLoaderType loader;
  //  loader.AddData(train_data, train_label);

  std::cout << "Build model & optimiser... " << std::endl;
  std::string                input_name, output_name, label_name, error_name;
  std::shared_ptr<GraphType> g = BuildModel(input_name, output_name, label_name, error_name);
  OptimiserType              optimiser(g, {input_name}, label_name, error_name);

  std::cout << "Begin training loop... " << std::endl;
  DataType loss;
  for (SizeType i{0}; i < epochs; i++)
  {
    loss = optimiser.Run({train_data}, train_label, batch_size);
    std::cout << "Loss: " << loss << std::endl;

    g->SetInput(input_name, test_data);
    auto prediction = g->Evaluate(output_name, false);
    prediction.Reshape({prediction.shape().at(1), prediction.shape().at(2)});

    // TODO: mean square error loss reports half the true MSE
    auto result = fetch::math::MeanSquareErrorLoss(prediction, test_label);
    std::cout << "mean square validation error: " << result << std::endl;

  }

  return 0;
}
