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

#include "dmlf/colearn/update_store_interface.hpp"

#include <queue>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/mutex.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class UpdateStore : public UpdateStoreInterface
{
public:
  UpdateStore()                         = default;
  ~UpdateStore() override               = default;
  UpdateStore(UpdateStore const &other) = delete;
  UpdateStore &operator=(UpdateStore const &other) = delete;

  void PushUpdate(ColearnURI const &uri, Data &&data, Metadata &&metadata) override;
  void PushUpdate(Algorithm const &algo, UpdateType type, Data &&data, Source source,
                  Metadata &&metadata) override;

  UpdatePtr GetUpdate(ColearnURI const &uri, Consumer consumer = "learner0") override;
  UpdatePtr GetUpdate(Algorithm const &algo, UpdateType const &type,
                      Consumer consumer = "learner0") override;

  UpdatePtr GetUpdate(ColearnURI const &uri, Criteria criteria,
                      Consumer consumer = "learner0") override;
  UpdatePtr GetUpdate(Algorithm const &algo, UpdateType const &type, Criteria criteria,
                      Consumer consumer = "learner0") override;

  std::size_t GetUpdateCount() const override;
  std::size_t GetUpdateCount(Algorithm const &algo, UpdateType const &type) const override;

private:
  using QueueId = std::string;
  QueueId Id(Algorithm const &algo, UpdateType const &type) const;

  Criteria Lifo = [](UpdatePtr const &update) -> double {
    return static_cast<double>(-update->TimeSinceCreation().count());
  };

  using Mutex           = fetch::Mutex;
  using Lock            = std::unique_lock<Mutex>;
  using Store           = std::vector<UpdatePtr>;
  using AlgoMap         = std::unordered_map<QueueId, Store>;
  using Fingerprint     = Update::Fingerprint;
  using UpdateConsumers = std::unordered_set<Consumer>;

  AlgoMap       algo_map_;
  mutable Mutex global_m_;

  std::unordered_map<Fingerprint, UpdateConsumers> consumed_;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
