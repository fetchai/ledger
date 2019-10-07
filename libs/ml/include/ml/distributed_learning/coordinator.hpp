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

#include "core/mutex.hpp"
#include "core/random.hpp"
#include "math/base_types.hpp"

#include <mutex>

namespace fetch {
namespace ml {
namespace distributed_learning {

template <class TensorType>
class TrainingClient;

enum class CoordinatorMode
{
  SYNCHRONOUS,
  SEMI_SYNCHRONOUS,
  ASYNCHRONOUS
};

enum class CoordinatorState
{
  RUN,
  STOP,
};

struct CoordinatorParams
{
  using SizeType = fetch::math::SizeType;

  CoordinatorMode mode;
  SizeType        iterations_count;
  SizeType        number_of_peers;
};

template <typename TensorType>
class Coordinator
{
public:
  using SizeType      = fetch::math::SizeType;
  using ClientPtrType = std::shared_ptr<TrainingClient<TensorType>>;

  explicit Coordinator(CoordinatorParams const &params);

  void IncrementIterationsCounter();

  void Reset();

  CoordinatorMode GetMode() const;

  CoordinatorState GetState() const;

  std::vector<std::shared_ptr<TrainingClient<TensorType>>> NextPeersList(std::string &client_id);

  void AddClient(ClientPtrType const &new_client);
  void SetClientsList(std::vector<ClientPtrType> const &new_clients);

private:
  CoordinatorMode                                          mode_;
  CoordinatorState                                         state_           = CoordinatorState::RUN;
  SizeType                                                 iterations_done_ = 0;
  SizeType                                                 iterations_count_;
  mutable std::mutex                                       iterations_mutex_;
  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients_;
  SizeType                                                 number_of_peers_;

  // random number generator for shuffling peers
  fetch::random::LaggedFibonacciGenerator<> gen_;
};

template <typename TensorType>
Coordinator<TensorType>::Coordinator(CoordinatorParams const &params)
  : mode_(params.mode)
  , iterations_count_(params.iterations_count)
  , number_of_peers_(params.number_of_peers)
{}

template <typename TensorType>
void Coordinator<TensorType>::IncrementIterationsCounter()
{
  FETCH_LOCK(iterations_mutex_);
  iterations_done_++;

  if (iterations_done_ >= iterations_count_)
  {
    state_ = CoordinatorState::STOP;
  }
}

template <typename TensorType>
void Coordinator<TensorType>::Reset()
{
  iterations_done_ = 0;
  state_           = CoordinatorState::RUN;
}

template <typename TensorType>
CoordinatorMode Coordinator<TensorType>::GetMode() const
{
  return mode_;
}

template <typename TensorType>
CoordinatorState Coordinator<TensorType>::GetState() const
{
  return state_;
}

/**
 * Add pointer to client
 * @param clients
 */
template <typename TensorType>
void Coordinator<TensorType>::AddClient(
    std::shared_ptr<TrainingClient<TensorType>> const &new_client)
{
  clients_.push_back(new_client);
}

/**
 * Add pointer to client
 * @param clients
 */
template <typename TensorType>
void Coordinator<TensorType>::SetClientsList(
    std::vector<std::shared_ptr<TrainingClient<TensorType>>> const &new_clients)
{
  clients_ = new_clients;
}

/**
 * Get list of new peers
 */
template <typename TensorType>
std::vector<std::shared_ptr<TrainingClient<TensorType>>> Coordinator<TensorType>::NextPeersList(
    std::string &client_id)
{
  std::vector<std::shared_ptr<TrainingClient<TensorType>>> shuffled_clients;

  for (auto &cl : clients_)
  {
    if (cl->GetId() == client_id)
    {
      continue;
    }
    shuffled_clients.push_back(cl);
  }

  // Shuffle the peers list to get new contact for next update
  fetch::random::Shuffle(gen_, shuffled_clients, shuffled_clients);

  // Create vector subset
  std::vector<std::shared_ptr<TrainingClient<TensorType>>> new_peers(
      shuffled_clients.begin(),
      shuffled_clients.begin() + static_cast<fetch::math::PtrDiffType>(number_of_peers_));

  return new_peers;
}

}  // namespace distributed_learning
}  // namespace ml
}  // namespace fetch
