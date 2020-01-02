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

#include "beacon/block_entropy.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include <memory>
#include <unordered_map>

namespace fetch {
namespace ledger {

class Block;
class StorageInterface;

class ConsensusInterface
{
public:
  using NextBlockPtr   = std::unique_ptr<Block>;
  using StakeSnapshot  = ledger::StakeSnapshot;
  using Minerwhitelist = beacon::BlockEntropy::Cabinet;

  enum class Status
  {
    YES,
    NO
  };

  ConsensusInterface()          = default;
  virtual ~ConsensusInterface() = default;

  // Let the consensus know which block you are on. Only valid
  // to update the current block incrementally forward but valid
  // to update backward any number
  virtual void UpdateCurrentBlock(Block const &current) = 0;

  // Populate the next block for packing and submission. Will return
  // an empty pointer if the miner should not emit a block
  virtual NextBlockPtr GenerateNextBlock() = 0;

  // Verify a block according to consensus requirements. It must not be loose.
  virtual Status ValidBlock(Block const &current) const = 0;

  // Set system parameters
  virtual void SetMaxCabinetSize(uint16_t max_cabinet_size)                    = 0;
  virtual void SetBlockInterval(uint64_t block_interval_s)                     = 0;
  virtual void SetAeonPeriod(uint16_t aeon_period)                             = 0;
  virtual void SetDefaultStartTime(uint64_t default_start_time_ms)             = 0;
  virtual void Reset(StakeSnapshot const &snapshot, StorageInterface &storage) = 0;
  virtual void Reset(StakeSnapshot const &snapshot)                            = 0;
  virtual void SetWhitelist(Minerwhitelist const &whitelist)                   = 0;
};

}  // namespace ledger
}  // namespace fetch
