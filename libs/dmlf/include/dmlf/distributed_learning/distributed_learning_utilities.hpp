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

/**
 * Averages weights between all clients
 * @param clients
 */
template <typename TensorType>
void SynchroniseWeights(
    std::vector<std::shared_ptr<fetch::ml::distributed_learning::TrainingClient<TensorType>>>
        clients)
{
  std::vector<TensorType> new_weights = clients[0]->GetWeights();

  // Sum all weights
  for (typename TensorType::SizeType i{1}; i < clients.size(); ++i)
  {
    std::vector<TensorType> other_weights = clients[i]->GetWeights();

    for (typename TensorType::SizeType j{0}; j < other_weights.size(); j++)
    {
      fetch::math::Add(new_weights.at(j), other_weights.at(j), new_weights.at(j));
    }
  }

  // Divide weights by number of clients to calculate the average
  for (typename TensorType::SizeType j{0}; j < new_weights.size(); j++)
  {
    fetch::math::Divide(new_weights.at(j), static_cast<typename TensorType::Type>(clients.size()),
                        new_weights.at(j));
  }

  // Update models of all clients by average model
  for (auto &c : clients)
  {
    c->SetWeights(new_weights);
  }
}
