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

#include "core/byte_array/byte_array.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
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
class StakeSnapshot;

class GenesisFileCreator
{
public:
  using ConsensusPtr     = std::shared_ptr<fetch::ledger::ConsensusInterface>;
  using ByteArray        = byte_array::ByteArray;
  using ConstByteArray   = byte_array::ConstByteArray;
  using CertificatePtr   = std::shared_ptr<crypto::Prover>;
  using GenesisStore     = fetch::storage::ObjectStore<Block>;
  using MainChain        = ledger::MainChain;
  using StakeSnapshotPtr = std::shared_ptr<StakeSnapshot>;

  enum class Result
  {
    FAILURE = 0,
    LOADED_PREVIOUS_GENESIS,
    CREATED_NEW_GENESIS
  };

  struct ConsensusParameters
  {
    uint16_t                      cabinet_size{0};
    uint64_t                      start_time{0};
    StakeSnapshotPtr              snapshot{};
    beacon::BlockEntropy::Cabinet whitelist;
  };

  // Construction / Destruction
  GenesisFileCreator(StorageUnitInterface &storage_unit, CertificatePtr certificate,
                     std::string const &db_prefix);
  GenesisFileCreator(GenesisFileCreator const &) = delete;
  GenesisFileCreator(GenesisFileCreator &&)      = delete;
  ~GenesisFileCreator()                          = default;

  Result LoadContents(ConstByteArray const &contents, bool proof_of_stake,
                      ConsensusParameters &params);

  // Operators
  GenesisFileCreator &operator=(GenesisFileCreator const &) = delete;
  GenesisFileCreator &operator=(GenesisFileCreator &&) = delete;

private:
  bool LoadState(variant::Variant const &object, ConsensusParameters const *consensus = nullptr);
  bool LoadConsensus(variant::Variant const &object, ConsensusParameters &params);

  CertificatePtr        certificate_;
  StorageUnitInterface &storage_unit_;
  GenesisStore          genesis_store_;
  Block                 genesis_block_;
  std::string           db_name_;
};

}  // namespace ledger
}  // namespace fetch
