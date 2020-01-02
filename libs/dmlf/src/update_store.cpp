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

#include <cmath>
#include <numeric>
#include <stdexcept>

#include "dmlf/colearn/update_store.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

UpdateStore::QueueId UpdateStore::Id(Algorithm const &algo, UpdateType const &type) const
{
  return algo + "->" + type;
}

std::size_t UpdateStore::GetUpdateCount() const
{
  FETCH_LOCK(global_m_);

  std::size_t sum = 0;
  for (auto p : algo_map_)
  {
    sum += p.second.size();
  }
  return sum;
}
std::size_t UpdateStore::GetUpdateCount(Algorithm const &algo, UpdateType const &type) const
{
  FETCH_LOCK(global_m_);
  QueueId id = Id(algo, type);

  auto it = algo_map_.find(id);
  if (it == algo_map_.end())
  {
    return 0;
  }
  return it->second.size();
}

void UpdateStore::PushUpdate(ColearnURI const &uri, Data &&data, Metadata &&metadata)
{
  if (uri.algorithm_class().empty())
  {
    throw std::invalid_argument("Uri is missing fields. Recieved \"" + uri.ToString() +
                                "\", algorithm_class must not be empty");
  }
  if (uri.update_type().empty())
  {
    throw std::invalid_argument("Uri is missing fields. Recieved \"" + uri.ToString() +
                                "\", update_type must not be empty");
  }
  if (uri.source().empty())
  {
    throw std::invalid_argument("Uri is missing fields. Recieved \"" + uri.ToString() +
                                "\", source must not be empty");
  }

  PushUpdate(uri.algorithm_class(), uri.update_type(), std::move(data), uri.source(),
             std::move(metadata));
}
void UpdateStore::PushUpdate(Algorithm const &algo, UpdateType type, Data &&data, Source source,
                             Metadata &&metadata)
{
  QueueId id        = Id(algo, type);
  auto    newUpdate = std::make_shared<Update>(algo, std::move(type), std::move(data),
                                            std::move(source), std::move(metadata));
  FETCH_LOCK(global_m_);

  auto result = consumed_.emplace(newUpdate->fingerprint(), UpdateConsumers());

  if (!result.second)  // Duplicate
  {
    return;
  }

  result.first->second.insert(newUpdate->source());

  auto queue_it = algo_map_.find(id);
  if (queue_it == algo_map_.end())
  {
    queue_it = algo_map_.emplace(id, Store{}).first;
  }
  auto &store = queue_it->second;

  store.emplace_back(newUpdate);
}

UpdateStore::UpdatePtr UpdateStore::GetUpdate(Algorithm const &algo, UpdateType const &type,
                                              Criteria criteria, Consumer consumer)
{
  QueueId id = Id(algo, type);
  FETCH_LOCK(global_m_);
  auto queue_it = algo_map_.find(id);

  if (queue_it == algo_map_.end() || queue_it->second.empty())
  {
    throw std::runtime_error("No updates of algo " + algo + " and type " + type + " in store\n");
  }

  auto wrapped_criteria = [&criteria, &consumer, &seen = consumed_](UpdatePtr const &update) {
    if (!consumer.empty() && seen[update->fingerprint()].count(consumer) == 1)
    {
      return std::nan("");
    }
    return criteria(update);
  };

  auto &store = queue_it->second;

  auto it = std::max_element(store.cbegin(), store.cend(),
                             [&wrapped_criteria](UpdatePtr const &left, UpdatePtr const &right) {
                               double leftScore = wrapped_criteria(left);
                               if (std::isnan(leftScore))
                               {
                                 return true;
                               }
                               double rightScore = wrapped_criteria(right);
                               if (std::isnan(rightScore))
                               {
                                 return false;
                               }

                               return leftScore < rightScore;
                             });

  auto result = *it;
  if (std::isnan(wrapped_criteria(result)))
  {
    throw std::runtime_error("No updates of algo " + algo + " and type " + type +
                             " matching the criteria found\n");
  }

  if (!consumer.empty())
  {
    consumed_[result->fingerprint()].insert(consumer);
  }

  return result;
}

UpdateStore::UpdatePtr UpdateStore::GetUpdate(ColearnURI const &uri, Criteria criteria,
                                              Consumer consumer)
{
  if (uri.algorithm_class().empty())
  {
    throw std::invalid_argument("Uri is missing fields. Recieved \"" + uri.ToString() +
                                "\", algorithm_class must not be empty");
  }
  if (uri.update_type().empty())
  {
    throw std::invalid_argument("Uri is missing fields. Recieved \"" + uri.ToString() +
                                "\", update_type must not be empty");
  }

  return GetUpdate(uri.algorithm_class(), uri.update_type(), criteria, consumer);
}
UpdateStore::UpdatePtr UpdateStore::GetUpdate(ColearnURI const &uri, Consumer consumer)
{
  if (uri.algorithm_class().empty())
  {
    throw std::invalid_argument("Uri is missing fields. Recieved \"" + uri.ToString() +
                                "\", algorithm_class must not be empty");
  }
  if (uri.update_type().empty())
  {
    throw std::invalid_argument("Uri is missing fields. Recieved \"" + uri.ToString() +
                                "\", update_type must not be empty");
  }

  return GetUpdate(uri.algorithm_class(), uri.update_type(), consumer);
}
UpdateStore::UpdatePtr UpdateStore::GetUpdate(Algorithm const &algo, UpdateType const &type,
                                              Consumer consumer)
{
  return GetUpdate(algo, type, Lifo, consumer);
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
