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
  StorageUnitBundledService()  = default;
  ~StorageUnitBundledService() = default;

  void Setup(std::string const &dbdir, std::size_t const &lanes, uint16_t const &port,
             fetch::network::NetworkManager const &tm)
  {
    for (std::size_t i = 0; i < lanes; ++i)
    {
      lanes_.push_back(std::make_shared<LaneService>(dbdir, uint32_t(i), lanes, uint16_t(port + i),
                                                     tm));
    }
  }

  void Start(bool start_sync = true)
  {
    for (auto &lane : lanes_)
    {
      lane->Start(start_sync);
    }
  }

private:
  std::vector<std::shared_ptr<LaneService>> lanes_;
};

}  // namespace ledger
}  // namespace fetch
