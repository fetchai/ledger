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
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

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
  std::shared_ptr<fetch::ml::Graph<TensorType>> g_ptr =
      std::make_shared<fetch::ml::Graph<TensorType>>();

  client_params.inputs_names = {
      g_ptr->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {})};
  g_ptr->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC1", {"Input"},
                                                                         28u * 28u, 10u);
  g_ptr->template AddNode<fetch::ml::ops::Relu<TensorType>>("Relu1", {"FC1"});
  g_ptr->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC2", {"Relu1"}, 10u,
                                                                         10u);
  g_ptr->template AddNode<fetch::ml::ops::Relu<TensorType>>("Relu2", {"FC2"});
  g_ptr->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC3", {"Relu2"}, 10u,
                                                                         10u);
  g_ptr->template AddNode<fetch::ml::ops::Softmax<TensorType>>("Softmax", {"FC3"});
  client_params.label_name =
      g_ptr->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});
  client_params.error_name = g_ptr->template AddNode<fetch::ml::ops::CrossEntropyLoss<TensorType>>(
      "Error", {"Softmax", "Label"});
  g_ptr->Compile();

  // Initialise DataLoader
  std::shared_ptr<fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>> dataloader_ptr =
      std::make_shared<fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>>(images, labels);
  dataloader_ptr->SetTestRatio(test_set_ratio);
  dataloader_ptr->SetRandomMode(true);
  // Initialise Optimiser
  std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>> optimiser_ptr =
      std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
          std::shared_ptr<fetch::ml::Graph<TensorType>>(g_ptr), client_params.inputs_names,
          client_params.label_name, client_params.error_name, client_params.learning_rate);

  return std::make_shared<fetch::dmlf::distributed_learning::TrainingClient<TensorType>>(
      id, g_ptr, dataloader_ptr, optimiser_ptr, client_params, console_mutex_ptr);
}

}  // namespace utilities
}  // namespace distributed_learning
}  // namespace dmlf
}  // namespace fetch
