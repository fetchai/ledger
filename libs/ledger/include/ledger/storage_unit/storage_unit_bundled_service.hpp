#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "storage/document_store_protocol.hpp"
#include "storage/object_store.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/object_store_syncronisation_protocol.hpp"
#include "storage/revertible_document_store.hpp"

#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

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

  void Setup(std::string const &storage_path, std::size_t const &lanes, uint16_t const &port,
             fetch::network::NetworkManager const &tm, bool refresh_storage = false)
  {
    for (std::size_t i = 0; i < lanes; ++i)
    {
      // Portion out memory for lanes (bytes)
      std::size_t max_memory_per_lane = std::size_t(1ULL << 31) / lanes;

      lanes_.push_back(std::make_shared<LaneService>(storage_path, uint32_t(i), lanes,
                                                     uint16_t(port + i), tm, max_memory_per_lane,
                                                     refresh_storage));
    }
  }

  void Start()
  {
    for (auto &lane : lanes_)
    {
      lane->Start();
    }
  }

  void Stop()
  {
    for (auto &lane : lanes_)
    {
      lane->Stop();
    }
  }

private:
  using LaneServicePtr  = std::shared_ptr<LaneService>;
  using LaneServiceList = std::vector<LaneServicePtr>;

  LaneServiceList lanes_;
};

}  // namespace ledger
}  // namespace fetch
