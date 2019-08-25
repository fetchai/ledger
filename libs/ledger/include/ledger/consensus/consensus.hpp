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

#include "crypto/identity.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/main_chain.hpp"

#include "beacon/beacon_service.hpp"
#include "beacon/beacon_setup_protocol.hpp"
#include "beacon/beacon_setup_service.hpp"
#include "beacon/cabinet_member_details.hpp"
#include "beacon/entropy.hpp"
#include "beacon/event_manager.hpp"

#include "ledger/consensus/stake_manager.hpp"

#include <unordered_map>
#include <cmath>

namespace fetch {
namespace ledger {

class Consensus final : public ConsensusInterface
{
public:
  using StakeManagerPtr        = std::shared_ptr<ledger::StakeManager>;
  using BeaconServicePtr       = std::shared_ptr<fetch::beacon::BeaconService>;
  using CabinetMemberList      = beacon::BeaconService::CabinetMemberList;
  using Identity               = crypto::Identity;
  using MainChain              = ledger::MainChain;

  Consensus(StakeManagerPtr stake, BeaconServicePtr beacon, MainChain const& chain, Identity mining_identity, uint64_t aeon_period, uint64_t max_committee_size);

  void UpdateCurrentBlock(Block const &current) override;
  NextBlockPtr GenerateNextBlock() override;
  Status ValidBlock(Block const &previous, Block const &current) override;
  void Refresh() override;

private:
  StakeManagerPtr            stake_;
  BeaconServicePtr           beacon_;
  MainChain const           &chain_;
  Identity                   mining_identity_;
  Address                    mining_address_;
  uint64_t                   aeon_period_        = 0;
  uint64_t                   max_committee_size_ = 0;
  double                     threshold_          = 1.0;
  uint64_t                   current_block_number_ = 0;
  int64_t                    last_committee_created_ = -1;
  Block                      current_block_;
};

}  // namespace ledger
}  // namespace fetch

