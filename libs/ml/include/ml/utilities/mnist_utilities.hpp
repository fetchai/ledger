#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/base_types.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/model/model_config.hpp"
#include "ml/model/sequential.hpp"
#include "ml/ops/activation.hpp"

namespace fetch {
namespace ml {
namespace utilities {

using SizeType = fetch::math::SizeType;

template <typename TensorType>
std::pair<TensorType, TensorType> generate_dummy_data(SizeType n_data)
{
  using DataType = typename TensorType::Type;

  // generate some random data for training and testing
  TensorType data{{28, 28, n_data}};
  data.FillUniformRandom();

  TensorType labels{{10, n_data}};
  labels.SetAllZero();
  for (std::size_t i = 0; i < n_data; ++i)
  {
    labels.Set(i % 10, i, DataType{1});
  }

  return std::make_pair(data, labels);
}

template <typename TensorType>
fetch::ml::model::Sequential<TensorType> setup_mnist_model(
    model::ModelConfig<typename TensorType::Type> &model_config, TensorType &data,
    TensorType &labels, fetch::fixed_point::fp32_t test_ratio = fetch::fixed_point::fp32_t{"0.2"})
{
  using ModelType      = typename fetch::ml::model::Sequential<TensorType>;
  using DataLoaderType = typename fetch::ml::dataloaders::TensorDataLoader<TensorType>;
  using OptimiserType  = fetch::ml::OptimiserType;
  using LossType       = fetch::ml::ops::LossType;

  // package data into data vector
  auto const data_vector = std::vector<TensorType>({data});

  // set up dataloader
  auto data_loader_ptr = std::make_unique<DataLoaderType>();
  data_loader_ptr->AddData(data_vector, labels);
  data_loader_ptr->SetTestRatio(test_ratio);

  // setup model and pass dataloader
  ModelType model(model_config);
  model.template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      784, 100, fetch::ml::details::ActivationType::RELU);
  model.template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      100, 20, fetch::ml::details::ActivationType::RELU);
  model.template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      20, 10, fetch::ml::details::ActivationType::SOFTMAX);

  model.SetDataloader(std::move(data_loader_ptr));
  model.Compile(OptimiserType::ADAM, LossType::CROSS_ENTROPY);

  return model;
}

/**
 * Reads the mnist image file into a Tensor
 * @param full_path Path to image file
 * @return Tensor with shape {28, 28, n_images}
 */
template <typename TensorType>
TensorType read_mnist_images(std::string const &full_path);

/**
 * Reads the mnist labels file into a Tensor
 * @param full_path Path to labels file
 * @return Tensor with shape {1, n_images}
 */
template <typename TensorType>
TensorType read_mnist_labels(std::string const &full_path);

template <typename TensorType>
TensorType convert_labels_to_onehot(TensorType labels);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
