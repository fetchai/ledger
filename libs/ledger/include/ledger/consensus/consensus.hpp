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

class Consensus final : public ConsensusInterface
{
public:
  using StakeManagerPtr   = std::shared_ptr<ledger::StakeManager>;
  using BeaconServicePtr  = std::shared_ptr<fetch::beacon::BeaconService>;
  using CabinetMemberList = beacon::BeaconService::CabinetMemberList;
  using Identity          = crypto::Identity;
  using WeightedQual      = std::vector<Identity>;
  using MainChain         = ledger::MainChain;
  using BlockEntropy      = ledger::Block::BlockEntropy;

  Consensus(StakeManagerPtr stake, BeaconServicePtr beacon, MainChain const &chain,
            Identity mining_identity, uint64_t aeon_period, uint64_t max_cabinet_size,
            uint64_t block_interval_ms = 1000);

  void         UpdateCurrentBlock(Block const &current) override;
  NextBlockPtr GenerateNextBlock() override;
  Status       ValidBlock(Block const &current) const override;
  void         Reset(StakeSnapshot const &snapshot);
  void         Refresh() override;

  StakeManagerPtr stake();
  void            SetThreshold(double threshold);
  void            SetCabinetSize(uint64_t size);
  void            SetDefaultStartTime(uint64_t default_start_time);

private:
  static constexpr std::size_t HISTORY_LENGTH = 1000;

  using Cabinet        = StakeManager::Cabinet;
  using CabinetPtr     = std::shared_ptr<Cabinet const>;
  using BlockIndex     = uint64_t;
  using CabinetHistory = std::map<BlockIndex, CabinetPtr>;

  StakeManagerPtr  stake_;
  BeaconServicePtr beacon_;
  MainChain const &chain_;
  Identity         mining_identity_;
  chain::Address   mining_address_;

  // Global variables relating to consensus
  uint64_t aeon_period_      = 0;
  uint64_t max_cabinet_size_ = 0;
  double   threshold_        = 1.0;

  // Consensus' view on the heaviest block etc.
  Block  current_block_;
  Block  previous_block_;
  Block  beginning_of_aeon_;
  Digest last_triggered_cabinet_;

  uint64_t       default_start_time_ = 0;
  CabinetHistory cabinet_history_{};  ///< Cache of historical cabinets
  uint64_t       block_interval_ms_{std::numeric_limits<uint64_t>::max()};

  CabinetPtr GetCabinet(Block const &previous);
  bool       ValidMinerForBlock(Block const &previous, chain::Address const &address);
  uint64_t   GetBlockGenerationWeight(Block const &previous, chain::Address const &address);
  bool       ValidBlockTiming(Block const &previous, Block const &proposed) const;
  bool       ShouldTriggerNewCabinet(Block const &block);
  bool       EnoughQualSigned(BlockEntropy const &block_entropy) const;
};

}  // namespace ledger
}  // namespace fetch
