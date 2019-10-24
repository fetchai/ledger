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

namespace fetch {
namespace ml {
namespace utilities {

template <typename TensorType>
std::shared_ptr<fetch::ml::distributed_learning::TrainingClient<TensorType>> MakeBostonClient(
    std::string                                                               id,
    fetch::ml::distributed_learning::ClientParams<typename TensorType::Type> &client_params,
    TensorType &data_tensor, TensorType &label_tensor, float test_set_ratio,
    std::shared_ptr<std::mutex> console_mutex_ptr)
{
  // Initialise model
  std::shared_ptr<fetch::ml::Graph<TensorType>> g_ptr =
      std::make_shared<fetch::ml::Graph<TensorType>>();

  client_params.inputs_names = {
      g_ptr->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {})};
  g_ptr->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC1", {"Input"}, 13u,
                                                                         10u);
  g_ptr->template AddNode<fetch::ml::ops::Relu<TensorType>>("Relu1", {"FC1"});
  g_ptr->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC2", {"Relu1"}, 10u,
                                                                         10u);
  g_ptr->template AddNode<fetch::ml::ops::Relu<TensorType>>("Relu2", {"FC2"});
  g_ptr->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC3", {"Relu2"}, 10u, 1u);
  client_params.label_name =
      g_ptr->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});
  client_params.error_name =
      g_ptr->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>("Error",
                                                                               {"FC3", "Label"});
  g_ptr->Compile();

  // Initialise DataLoader
  std::shared_ptr<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>> dataloader_ptr =
      std::make_shared<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>>();
  dataloader_ptr->AddData(data_tensor, label_tensor);

  dataloader_ptr->SetTestRatio(test_set_ratio);
  dataloader_ptr->SetRandomMode(true);
  // Initialise Optimiser
  std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>> optimiser_ptr =
      std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
          std::shared_ptr<fetch::ml::Graph<TensorType>>(g_ptr), client_params.inputs_names,
          client_params.label_name, client_params.error_name, client_params.learning_rate);

  return std::make_shared<fetch::ml::distributed_learning::TrainingClient<TensorType>>(
      id, g_ptr, dataloader_ptr, optimiser_ptr, client_params, console_mutex_ptr);
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
