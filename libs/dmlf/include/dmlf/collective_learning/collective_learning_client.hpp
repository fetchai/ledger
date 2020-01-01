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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "dmlf/collective_learning/client_algorithm.hpp"
#include "dmlf/collective_learning/client_algorithm_controller.hpp"
#include "dmlf/collective_learning/client_params.hpp"
#include "dmlf/deprecated/abstract_learner_networker.hpp"
#include "dmlf/deprecated/update.hpp"

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fetch {
namespace dmlf {
namespace collective_learning {

template <class TensorType>
class CollectiveLearningClient
{
  using DataType                = typename TensorType::Type;
  using AlgorithmControllerType = ClientAlgorithmController<TensorType>;
  using AlgorithmPtrType        = std::shared_ptr<ClientAlgorithm<TensorType>>;

public:
  CollectiveLearningClient(std::string id, ClientParams<DataType> const &client_params,
                           std::shared_ptr<dmlf::deprecated_AbstractLearnerNetworker> networker_ptr,
                           std::shared_ptr<std::mutex> console_mutex_ptr,
                           bool                        build_algorithms = true);
  virtual ~CollectiveLearningClient() = default;

  void RunAlgorithms(std::vector<std::thread> &threads);
  void RunAlgorithms();

  std::vector<AlgorithmPtrType> GetAlgorithms();
  DataType                      GetLossAverage();

  template <typename AlgorithmType    = ClientAlgorithm<TensorType>,
            typename ClientParamsType = ClientParams<TensorType>>
  void BuildAlgorithms(ClientParamsType const &    client_params,
                       std::shared_ptr<std::mutex> console_mutex_ptr);

protected:
  std::string                              id_;
  std::shared_ptr<AlgorithmControllerType> algorithm_controller_;
  std::vector<AlgorithmPtrType>            algorithms_;
};

template <class TensorType>
CollectiveLearningClient<TensorType>::CollectiveLearningClient(
    std::string id, ClientParams<DataType> const &client_params,
    std::shared_ptr<dmlf::deprecated_AbstractLearnerNetworker> networker_ptr,
    std::shared_ptr<std::mutex> console_mutex_ptr, bool build_algorithms)
  : id_(std::move(id))
{
  // build algorithm controller
  algorithm_controller_ = std::make_shared<AlgorithmControllerType>(networker_ptr);

  if (build_algorithms)
  {
    // build algorithms
    BuildAlgorithms(client_params, console_mutex_ptr);
  }
}

template <class TensorType>
template <typename AlgorithmType, typename ClientParamsType>
void CollectiveLearningClient<TensorType>::BuildAlgorithms(
    ClientParamsType const &client_params, std::shared_ptr<std::mutex> console_mutex_ptr)
{
  for (std::size_t i = 0; i < client_params.n_algorithms_per_client; ++i)
  {
    algorithms_.emplace_back(std::make_shared<AlgorithmType>(
        algorithm_controller_, "client" + id_ + "_algo" + std::to_string(i), client_params,
        console_mutex_ptr));
  }
}

template <class TensorType>
void CollectiveLearningClient<TensorType>::RunAlgorithms(std::vector<std::thread> &threads)
{
  // begin all client owned algorithms running
  for (auto &algorithm : algorithms_)
  {
    threads.emplace_back([&algorithm] { algorithm->Run(); });
  }
}

/**
 * Single threaded implementation that runs algorithms sequentially
 * @tparam TensorType
 */
template <class TensorType>
void CollectiveLearningClient<TensorType>::RunAlgorithms()
{
  // begin all client owned algorithms running
  for (auto &algorithm : algorithms_)
  {
    algorithm->Run();
  }
}

template <class TensorType>
std::vector<typename CollectiveLearningClient<TensorType>::AlgorithmPtrType>
CollectiveLearningClient<TensorType>::GetAlgorithms()
{
  std::vector<AlgorithmPtrType> ret = algorithms_;
  return ret;
}

template <class TensorType>
typename CollectiveLearningClient<TensorType>::DataType
CollectiveLearningClient<TensorType>::GetLossAverage()
{
  DataType loss_mean(0);
  for (auto const &algorithm : algorithms_)
  {
    loss_mean += algorithm->GetLossAverage();
  }
  loss_mean /= static_cast<DataType>(algorithms_.size());
  return loss_mean;
}

}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
