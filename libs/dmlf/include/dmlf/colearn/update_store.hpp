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

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "core/mutex.hpp"

#include "dmlf/colearn/colearn_update.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class UpdateStore
{
public:
  using Update    = ColearnUpdate;
  using UpdatePtr = std::shared_ptr<Update>;
  using Algorithm = std::string;

  using UpdateType = Update::UpdateType;
  using Data       = Update::Data;
  using Source     = Update::Source;

  UpdateStore()                         = default;
  ~UpdateStore()                        = default;
  UpdateStore(UpdateStore const &other) = delete;
  UpdateStore &operator=(UpdateStore const &other) = delete;

  void        PushUpdate(Algorithm const &algo, UpdateType type, Data data, Source source);
  UpdatePtr   GetUpdate(Algorithm const &algo, UpdateType const &type);
  std::size_t GetUpdateCount() const;
  std::size_t GetUpdateCount(Algorithm const &algo, UpdateType const &type) const;

private:
  using QueueId = std::string;
  QueueId Id(Algorithm const &algo, UpdateType const &type) const;

  struct QueueOrder
  {
    bool operator()(UpdatePtr const &l, UpdatePtr const &r)
    {
      return l->time_stamp > r->time_stamp;
    }
  };

  using Mutex   = fetch::Mutex;
  using Lock    = std::unique_lock<Mutex>;
  using Queue   = std::priority_queue<UpdatePtr, std::vector<UpdatePtr>, QueueOrder>;
  using AlgoMap = std::unordered_map<QueueId, Queue>;

  AlgoMap       algo_map_;
  mutable Mutex global_m_;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
