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

#include "crypto/identity.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/consensus/consensus_interface.hpp"
#include "ledger/consensus/stake_manager.hpp"

namespace fetch {
namespace ledger {

class SimulatedPowConsensus final : public ConsensusInterface
{
public:
  using Identity = crypto::Identity;
  using Block    = ledger::Block;

  // Construction / Destruction
  SimulatedPowConsensus(Identity mining_identity, uint64_t block_interval_ms);

  SimulatedPowConsensus(SimulatedPowConsensus const &) = delete;
  SimulatedPowConsensus(SimulatedPowConsensus &&)      = delete;
  ~SimulatedPowConsensus() override                    = default;

  void TriggerBlockGeneration();

  // Overridden methods
  void         UpdateCurrentBlock(Block const &current) override;
  NextBlockPtr GenerateNextBlock() override;
  Status       ValidBlock(Block const &current) const override;

  // Methods used in POS, and so do nothing here
  void SetMaxCabinetSize(uint16_t max_cabinet_size) override;
  void SetBlockInterval(uint64_t block_interval_ms) override;
  void SetAeonPeriod(uint16_t aeon_period) override;
  void Reset(StakeSnapshot const &snapshot, StorageInterface &storage) override;
  void SetDefaultStartTime(uint64_t default_start_time) override;

  // Operators
  SimulatedPowConsensus &operator=(SimulatedPowConsensus const &) = delete;
  SimulatedPowConsensus &operator=(SimulatedPowConsensus &&) = delete;

private:
  Identity mining_identity_;

  // Recalculated whenever we see a new block: set a time for when we will produce
  // the next block
  uint64_t decided_next_timestamp_ms_{std::numeric_limits<uint64_t>::max()};

  // Consensus' view on the heaviest block etc.
  Block             current_block_;
  uint64_t          block_interval_ms_{std::numeric_limits<uint64_t>::max()};
  std::atomic<bool> forcibly_generate_next_{false};
};

}  // namespace ledger
}  // namespace fetch
