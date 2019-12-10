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

#include "dmlf/collective_learning/collective_learning_client.hpp"
#include "dmlf/deprecated/abstract_learner_networker.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/meta/ml_type_traits.hpp"

namespace fetch {
namespace dmlf {
namespace collective_learning {
namespace utilities {

namespace details {

/**
 * Utility for setting up a single model
 * @tparam TensorType
 * @param data
 * @param labels
 * @param test_set_ratio
 * @return
 */
template <typename TensorType>
std::shared_ptr<fetch::ml::model::Sequential<TensorType>> MakeBostonModel(TensorType &data,
                                                                          TensorType &labels,
                                                                          float test_set_ratio)
{
  // Initialise model
  auto model_ptr = std::make_shared<fetch::ml::model::Sequential<TensorType>>();

  model_ptr->template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      13u, 10u, fetch::ml::details::ActivationType::RELU);
  model_ptr->template Add<fetch::ml::layers::FullyConnected<TensorType>>(
      10u, 10u, fetch::ml::details::ActivationType::RELU);
  model_ptr->template Add<fetch::ml::layers::FullyConnected<TensorType>>(10u, 1u);

  // Initialise DataLoader
  auto dataloader_ptr =
      std::make_unique<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>>();
  dataloader_ptr->AddData({data}, labels);
  dataloader_ptr->SetTestRatio(test_set_ratio);
  dataloader_ptr->SetRandomMode(true);

  model_ptr->SetDataloader(std::move(dataloader_ptr));
  model_ptr->Compile(fetch::ml::OptimiserType::ADAM, fetch::ml::ops::LossType::MEAN_SQUARE_ERROR);

  return model_ptr;
}
}  // namespace details

template <typename TensorType>
std::shared_ptr<fetch::dmlf::collective_learning::CollectiveLearningClient<TensorType>>
MakeBostonClient(
    std::string                                                                id,
    fetch::dmlf::collective_learning::ClientParams<typename TensorType::Type> &client_params,
    TensorType &data, TensorType &labels, float test_set_ratio,
    std::shared_ptr<deprecated_AbstractLearnerNetworker> networker,
    std::shared_ptr<std::mutex>                          console_mutex_ptr)
{

  // set up the client first
  auto client = std::make_shared<CollectiveLearningClient<TensorType>>(id, client_params, networker,
                                                                       console_mutex_ptr);

  // build a boston model for each algorithm in the client
  auto algorithms = client->GetAlgorithms();
  for (auto const &algorithm : algorithms)
  {
    // build the model
    auto model_ptr = details::MakeBostonModel<TensorType>(data, labels, test_set_ratio);

    algorithm->SetModel(model_ptr);
  }

  return client;
}

}  // namespace utilities
}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
