#pragma once
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

#include "dmlf/distributed_learning/distributed_learning_client.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/utilities/mnist_utilities.hpp"

namespace fetch {
namespace dmlf {
namespace distributed_learning {
namespace utilities {

template <typename TensorType>
std::shared_ptr<fetch::dmlf::distributed_learning::TrainingClient<TensorType>> MakeMNISTClient(
    std::string const &                                                         id,
    fetch::dmlf::distributed_learning::ClientParams<typename TensorType::Type> &client_params,
    std::string const &images, std::string const &labels, float test_set_ratio,
    std::shared_ptr<std::mutex> console_mutex_ptr)
{
  // Initialise model
  auto model_ptr = std::make_shared<fetch::ml::model::Sequential<TensorType>>();

  model_ptr->template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      28u * 28u, 10u, fetch::ml::details::ActivationType::RELU);
  model_ptr->template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      10u, 10u, fetch::ml::details::ActivationType::RELU);
  model_ptr->template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      10u, 10u, fetch::ml::details::ActivationType::SOFTMAX);

  // Initialise DataLoader
  auto mnist_images = fetch::ml::utilities::read_mnist_images<TensorType>(images);
  auto mnist_labels = fetch::ml::utilities::read_mnist_labels<TensorType>(labels);
  mnist_labels      = fetch::ml::utilities::convert_labels_to_onehot(mnist_labels);

  auto dataloader_ptr =
      std::make_unique<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>>();
  dataloader_ptr->AddData({mnist_images}, mnist_labels);
  dataloader_ptr->SetTestRatio(test_set_ratio);
  dataloader_ptr->SetRandomMode(true);

  model_ptr->SetDataloader(std::move(dataloader_ptr));
  model_ptr->Compile(fetch::ml::OptimiserType::ADAM, fetch::ml::ops::LossType::CROSS_ENTROPY);

  // N.B. some names are not set until AFTER the model is compiled
  client_params.inputs_names = {model_ptr->InputName()};
  client_params.label_name   = model_ptr->LabelName();
  client_params.error_name   = model_ptr->ErrorName();

  return std::make_shared<TrainingClient<TensorType>>(id, model_ptr, client_params,
                                                      console_mutex_ptr);
}

}  // namespace utilities
}  // namespace distributed_learning
}  // namespace dmlf
}  // namespace fetch
