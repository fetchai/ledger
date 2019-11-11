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

void UpdateStore::PushUpdate(Algorithm const &algo, UpdateType type, Data &&data,
                             Source source, Metadata &&metadata)
{
  FETCH_LOCK(global_m_);

  QueueId id       = Id(algo, type);
  auto    queue_it = algo_map_.find(id);

  if (queue_it == algo_map_.end())
  {
    queue_it = algo_map_.emplace(id, Queue()).first;
  }

  queue_it->second.push(std::make_shared<Update>(std::move(algo), std::move(type),
                                                 std::move(data), std::move(source),
                                                 std::move(metadata)));
}
UpdateStore::UpdatePtr UpdateStore::GetUpdate(Algorithm const &algo, UpdateType const &type,
                                              Criteria /*criteria*/)
{
  return GetUpdate(algo, type);
}
UpdateStore::UpdatePtr UpdateStore::GetUpdate(Algorithm const &algo, UpdateType const &type)
{
  FETCH_LOCK(global_m_);
  QueueId id       = Id(algo, type);
  auto    queue_it = algo_map_.find(id);

  if (queue_it == algo_map_.end() || queue_it->second.empty())
  {
    throw std::runtime_error("No updates found");
  }

  auto result = queue_it->second.top();
  queue_it->second.pop();

  return result;
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
