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

#include <utility>

#include "dmlf/deprecated/abstract_learner_networker.hpp"
#include "dmlf/deprecated/update.hpp"

namespace fetch {
namespace dmlf {
namespace collective_learning {

template <class TensorType>
class ClientAlgorithmController
{
  using UpdateType                    = fetch::dmlf::deprecated_Update<TensorType>;
  using MessageControllerInterfacePtr = std::shared_ptr<dmlf::deprecated_AbstractLearnerNetworker>;

public:
  explicit ClientAlgorithmController(MessageControllerInterfacePtr mci_ptr);
  virtual ~ClientAlgorithmController() = default;

  template <typename UpdateType>
  std::shared_ptr<UpdateType> GetUpdate();
  void                        PushUpdate(std::shared_ptr<UpdateType> update);
  std::size_t                 UpdateCount();

protected:
private:
  MessageControllerInterfacePtr mci_ptr_;
  mutable std::mutex            algorithm_controller_mutex;
};

template <typename TensorType>
ClientAlgorithmController<TensorType>::ClientAlgorithmController(
    MessageControllerInterfacePtr mci_ptr)
  : mci_ptr_(std::move(mci_ptr))
{}

/**
 * This function handles logic for what to do with new updates provided by an algorithm.
 * In the current implementation this is simply immediately passed on to the message control
 * interface
 * @tparam TensorType
 * @param update
 */
template <typename TensorType>
void ClientAlgorithmController<TensorType>::PushUpdate(
    std::shared_ptr<ClientAlgorithmController::UpdateType> update)
{
  FETCH_LOCK(algorithm_controller_mutex);
  mci_ptr_->PushUpdate(update);
}

/**
 * reports how many updates are waiting to be collected by the calling algorithm
 * @tparam TensorType
 */
template <typename TensorType>
std::size_t ClientAlgorithmController<TensorType>::UpdateCount()
{
  FETCH_LOCK(algorithm_controller_mutex);
  return mci_ptr_->GetUpdateCount();
}

/**
 * gives the next update to the calling algorithm
 */
template <typename TensorType>
template <typename UpdateType>
std::shared_ptr<UpdateType> ClientAlgorithmController<TensorType>::GetUpdate()
{
  FETCH_LOCK(algorithm_controller_mutex);
  return mci_ptr_->GetUpdate<UpdateType>();
}

}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
