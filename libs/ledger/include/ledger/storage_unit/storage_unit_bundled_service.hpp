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

#include "ledger/shard_config.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace fetch {
namespace ledger {

class StorageUnitBundledService
{
public:
  // Construction / Destruction
  StorageUnitBundledService()                                  = default;
  StorageUnitBundledService(StorageUnitBundledService const &) = delete;
  StorageUnitBundledService(StorageUnitBundledService &&)      = delete;
  ~StorageUnitBundledService()                                 = default;

  // Operators
  StorageUnitBundledService &operator=(StorageUnitBundledService const &) = delete;
  StorageUnitBundledService &operator=(StorageUnitBundledService &&) = delete;

  using NetworkManager = network::NetworkManager;
  using Mode           = LaneService::Mode;

  void Setup(NetworkManager const &mgr, ShardConfigs const &configs,
             Mode mode = Mode::LOAD_DATABASE)
  {
    // create all the lane pointers
    lanes_.resize(configs.size());

    for (std::size_t i = 0; i < configs.size(); ++i)
    {
      lanes_[i] = std::make_shared<LaneService>(mgr, configs[i], mode);
    }
  }

  void StartInternal()
  {
    for (auto &lane : lanes_)
    {
      lane->StartInternal();
    }
  }

  void StartExternal()
  {
    for (auto &lane : lanes_)
    {
      lane->StartExternal();
    }
  }

  void StopExternal()
  {
    for (auto &lane : lanes_)
    {
      lane->StopExternal();
    }
  }

  void StopInternal()
  {
    for (auto &lane : lanes_)
    {
      lane->StopInternal();
    }

    lanes_.clear();
  }

private:
  using LaneServicePtr  = std::shared_ptr<LaneService>;
  using LaneServiceList = std::vector<LaneServicePtr>;

  LaneServiceList lanes_;
};

}  // namespace ledger
}  // namespace fetch
