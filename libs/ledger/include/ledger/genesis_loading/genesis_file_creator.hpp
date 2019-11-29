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

#include "core/byte_array/byte_array.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/consensus/consensus.hpp"
#include "ledger/consensus/consensus_interface.hpp"
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
  using ConsensusPtr   = std::shared_ptr<fetch::ledger::ConsensusInterface>;
  using ByteArray      = byte_array::ByteArray;
  using CertificatePtr = std::shared_ptr<crypto::Prover>;
  using GenesisStore   = fetch::storage::ObjectStore<Block>;

  // Construction / Destruction
  GenesisFileCreator(BlockCoordinator &block_coordinator, StorageUnitInterface &storage_unit,
                     ConsensusPtr consensus, CertificatePtr certificate,
                     std::string const &db_prefix);
  GenesisFileCreator(GenesisFileCreator const &) = delete;
  GenesisFileCreator(GenesisFileCreator &&)      = delete;
  ~GenesisFileCreator()                          = default;

  bool LoadFile(std::string const &name);

  // Operators
  GenesisFileCreator &operator=(GenesisFileCreator const &) = delete;
  GenesisFileCreator &operator=(GenesisFileCreator &&) = delete;

private:
  bool LoadState(variant::Variant const &object);
  bool LoadConsensus(variant::Variant const &object);

  CertificatePtr        certificate_;
  BlockCoordinator &    block_coordinator_;
  StorageUnitInterface &storage_unit_;
  ConsensusPtr          consensus_;
  uint64_t              start_time_ = 0;
  GenesisStore          genesis_store_;
  Block                 genesis_block_;
  bool                  loaded_genesis_{false};
  std::string           db_name_;
};

}  // namespace ledger
}  // namespace fetch
