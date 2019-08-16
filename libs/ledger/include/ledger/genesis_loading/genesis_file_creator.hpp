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

#include <string>

namespace fetch {
namespace variant {
class Variant;
}

namespace ledger {

class StakeManager;
class BlockCoordinator;
class StorageUnitInterface;

class GenesisFileCreator
{
public:
  // Construction / Destruction
  GenesisFileCreator(BlockCoordinator &block_coordinator, StorageUnitInterface &storage_unit,
                     StakeManager *stake_manager);
  GenesisFileCreator(GenesisFileCreator const &) = delete;
  GenesisFileCreator(GenesisFileCreator &&)      = delete;
  ~GenesisFileCreator()                          = default;

  void LoadFile(std::string const &name);

  // Operators
  GenesisFileCreator &operator=(GenesisFileCreator const &) = delete;
  GenesisFileCreator &operator=(GenesisFileCreator &&) = delete;

private:
  void LoadState(variant::Variant const &object);
  void LoadStake(variant::Variant const &object);

  BlockCoordinator &    block_coordinator_;
  StorageUnitInterface &storage_unit_;
  StakeManager *        stake_manager_{nullptr};
};

inline GenesisFileCreator::GenesisFileCreator(BlockCoordinator &    block_coordinator,
                                              StorageUnitInterface &storage_unit,
                                              StakeManager *        stake_manager)
  : block_coordinator_{block_coordinator}
  , storage_unit_{storage_unit}
  , stake_manager_{stake_manager}
{}

}  // namespace ledger
}  // namespace fetch
