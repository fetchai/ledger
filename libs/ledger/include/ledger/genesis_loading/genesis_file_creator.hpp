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

#include "ledger/chain/block.hpp"
#include "ledger/consensus/consensus.hpp"
#include "storage/object_store.hpp"

#include <string>

namespace fetch {
namespace variant {
class Variant;
}

namespace ledger {

class BlockCoordinator;
class StorageUnitInterface;

class GenesisFileCreator
{
public:
  using ConsensusPtr = std::shared_ptr<fetch::ledger::Consensus>;

  // Construction / Destruction
  GenesisFileCreator(BlockCoordinator &block_coordinator, StorageUnitInterface &storage_unit,
                     ConsensusPtr consensus);
  GenesisFileCreator(GenesisFileCreator const &) = delete;
  GenesisFileCreator(GenesisFileCreator &&)      = delete;
  ~GenesisFileCreator()                          = default;

  void LoadFile(std::string const &name);

  // Operators
  GenesisFileCreator &operator=(GenesisFileCreator const &) = delete;
  GenesisFileCreator &operator=(GenesisFileCreator &&) = delete;

private:
  using SavedGenesis = fetch::storage::ObjectStore<Block>;

  void LoadState(variant::Variant const &object);
  void LoadConsensus(variant::Variant const &object);

  BlockCoordinator &    block_coordinator_;
  StorageUnitInterface &storage_unit_;
  ConsensusPtr          consensus_;
  uint64_t              start_time_ = 0;
  SavedGenesis          saved_genesis_;
};

}  // namespace ledger
}  // namespace fetch
