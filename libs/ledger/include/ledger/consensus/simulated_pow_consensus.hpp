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

#include "ledger/consensus/consensus_interface.hpp"
#include "ledger/protocols/notarisation_service.hpp"

#include "chain/address.hpp"
#include "crypto/identity.hpp"
#include "ledger/chain/main_chain.hpp"

#include "beacon/beacon_service.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/event_manager.hpp"

#include "ledger/consensus/stake_manager.hpp"

#include <cmath>
#include <unordered_map>

namespace fetch {
namespace ledger {

class StorageInterface;

class SimulatedPowConsensus final : public ConsensusInterface
{
public:

  // Construction / Destruction
  Consensus(StakeManagerPtr stake, BeaconSetupServicePtr beacon_setup, BeaconServicePtr beacon,
            MainChain const &chain, StorageInterface &storage, Identity mining_identity,
            uint64_t aeon_period, uint64_t max_cabinet_size, uint64_t block_interval_ms = 1000,
            NotarisationPtr notarisation = NotarisationPtr{});
  Consensus(Consensus const &) = delete;
  Consensus(Consensus &&)      = delete;
  ~Consensus() override        = default;

  // Overridden methods
  void         UpdateCurrentBlock(Block const &current) override;
  NextBlockPtr GenerateNextBlock() override;
  Status       ValidBlock(Block const &current) const override;
  void         Refresh() override;

  // Operators
  Consensus &operator=(Consensus const &) = delete;
  Consensus &operator=(Consensus &&) = delete;

private:
  //StorageInterface &    storage_;
  //StakeManagerPtr       stake_;
  //BeaconSetupServicePtr cabinet_creator_;
  //BeaconServicePtr      beacon_;
  MainChain const &     chain_;
  Identity              mining_identity_;
  chain::Address        mining_address_;

  // Consensus' view on the heaviest block etc.
  Block          current_block_;
  Block          previous_block_;
  uint64_t       block_interval_ms_{std::numeric_limits<uint64_t>::max()};
};

}  // namespace ledger
}  // namespace fetch

