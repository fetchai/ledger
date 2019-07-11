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

#include "vectorise/fixed_point/fixed_point.hpp"

#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"

#include "ml/optimisation/adam_optimiser.hpp"
//#include "ml/optimisation/sgd_optimiser.hpp"

#include <iostream>
#include <ml/optimisation/sgd_optimiser.hpp>
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
  SizeType epochs{1};
  SizeType batch_size{8};
  bool     normalise = true;
};

class DataNormaliser
{

public:

  // set up min, max, and range tensors
  TensorType x_min;
  TensorType x_max;
  TensorType x_range;

  DataNormaliser() = default;

  /**
   * calculate the min, max, and range for reference data
   * @param reference_tensor
   */
  void SetScale(TensorType & reference_tensor)
  {
    x_min = TensorType({1, reference_tensor.shape(1)});
    x_max = TensorType({1, reference_tensor.shape(1)});
    x_range = TensorType({1, reference_tensor.shape(1)});

    x_min.Fill(fetch::math::numeric_max<DataType>());
    x_max.Fill(fetch::math::numeric_lowest<DataType>());
    x_range.Fill(fetch::math::numeric_lowest<DataType>());

    // calculate min, max, and range of each feature
    for (std::size_t i = 0; i < reference_tensor.shape(2); ++i)
    {
      auto x_min_it   = x_min.begin();
      auto x_max_it   = x_max.begin();
      auto x_range_it = x_range.begin();
      auto ref_it     = reference_tensor.Slice(i, 2).begin();
      while (x_min_it.is_valid())
      {
        if (*x_min_it > *ref_it)
        {
          *x_min_it = *ref_it;
        }

        if (*x_max_it < *ref_it)
        {
          *x_max_it = *ref_it;
        }

        if (*x_range_it < (*x_max_it - *x_min_it))
        {
          *x_range_it = (*x_max_it - *x_min_it);
        }

        ++ref_it;
        ++x_min_it;
        ++x_max_it;
        ++x_range_it;
      }
    }
  }

  /**
   * normalise tensor data with respect to reference data
   * @return
   */
  void Normalise(TensorType const & input_tensor, TensorType & output_tensor)
  {
    output_tensor.Reshape(input_tensor.shape());
    SizeType batch_dim = input_tensor.shape().size() - 1;

    // apply normalisation to each feature according to scale -1, 1
    for (std::size_t i = 0; i < input_tensor.shape(2); ++i)
    {
      auto x_min_it   = x_min.begin();
      auto x_range_it = x_range.begin();
      auto in_it      = input_tensor.Slice(i, batch_dim).begin();
      auto ret_it     = output_tensor.Slice(i, batch_dim).begin();
      while (ret_it.is_valid())
      {
        *ret_it = (*in_it - *x_min_it) / (*x_range_it);

        ++in_it;
        ++ret_it;
        ++x_min_it;
        ++x_range_it;
      }
    }
  }

  /**
   * denormalise tensor data with respect to reference data
   */
  void DeNormalise(TensorType const & input_tensor, TensorType & output_tensor)
  {
    output_tensor.Reshape(input_tensor.shape());
    SizeType batch_dim = input_tensor.shape().size() - 1;

    // apply normalisation to each feature according to scale -1, 1
    for (std::size_t i = 0; i < input_tensor.shape(2); ++i)
    {
      auto x_min_it   = x_min.begin();
      auto x_range_it = x_range.begin();
      auto in_it      = input_tensor.Slice(i, batch_dim).begin();
      auto ret_it     = output_tensor.Slice(i, batch_dim).begin();
      while (ret_it.is_valid())
      {
        *ret_it = ((*in_it) * (*x_range_it)) + *x_min_it;

        ++in_it;
        ++ret_it;
        ++x_min_it;
        ++x_range_it;
      }
    }
  }
};

std::shared_ptr<GraphType> BuildModel(std::string &input_name, std::string &output_name,
                                      std::string &label_name, std::string &error_name)
{
  auto g = std::make_shared<GraphType>();

  SizeType conv1D_1_filters        = 8;
  SizeType conv1D_1_input_channels = 1;
  SizeType conv1D_1_kernel_size    = 80;
  SizeType conv1D_1_stride         = 1;

  typename TensorType::Type keep_prob{1.0};

  SizeType conv1D_2_filters        = 1;
  SizeType conv1D_2_input_channels = conv1D_1_filters;
  SizeType conv1D_2_kernel_size    = 50;
  SizeType conv1D_2_stride         = 1;

  input_name = g->AddNode<PlaceHolder<TensorType>>("Input", {});
  label_name = g->AddNode<PlaceHolder<TensorType>>("Label", {});

  std::string layer_1 = g->AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      "Conv1D_1", {input_name}, conv1D_1_filters, conv1D_1_input_channels, conv1D_1_kernel_size,
      conv1D_1_stride, fetch::ml::details::ActivationType::RELU);

  std::string layer_2 = g->AddNode<Dropout<TensorType>>("Dropout_1", {layer_1}, keep_prob);

  output_name = g->AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
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

int main(int ac, char **av)
{
  if (ac < 5)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-data PATH/TO/train-labels PATH/TO/test-data PATH/TO/test-labels"
              << std::endl;
    return 1;
  }

  TrainingParams tp;
  DataNormaliser scaler;

  std::cout << "FETCH Crypto price prediction demo" << std::endl;

  std::cout << "Loading crypto price data... " << std::endl;
  std::vector<TensorType> data_and_labels = LoadData(av[1], av[2], av[3], av[4]);
  TensorType              orig_train_data = data_and_labels.at(0);
  TensorType              orig_train_label     = data_and_labels.at(1);
  TensorType              orig_test_data       = data_and_labels.at(2);
  TensorType              orig_test_label      = data_and_labels.at(3);
  TensorType train_data(orig_train_data);
  TensorType train_label(orig_train_label);
  TensorType test_data(orig_test_data);
  TensorType test_label(orig_test_label);
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
  OptimiserType              optimiser(g, {input_name}, label_name, error_name);

  std::cout << "Begin training loop... " << std::endl;
  DataType loss;
  for (SizeType i{0}; i < tp.epochs; i++)
  {
    loss = optimiser.Run(loader, tp.batch_size);

    g->SetInput(input_name, test_data);
    auto prediction = g->Evaluate(output_name, false);
    prediction.Reshape({prediction.shape().at(1), prediction.shape().at(2)});

    if (tp.normalise)
    {
      scaler.DeNormalise(prediction, prediction);
    }
    auto result = fetch::math::MeanSquareError(prediction, orig_test_label);
    std::cout << "mean square validation error: " << result << std::endl;
  }

  return 0;
}
