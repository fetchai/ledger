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
#include "ledger/consensus/entropy_generator_interface.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/stake_snapshot.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace fetch {
namespace ledger {
constexpr char const *LOGGING_NAME = "StakeMgr";

StakeManager::StakeManager(uint64_t committee_size)
  : committee_size_{committee_size}
{}

void StakeManager::UpdateCurrentBlock(Block const &current)
{
  // The first stake can only be set through a reset event
  if (current.body.block_number == 0)
  {
    return;
  }

  // need to evaluate any of the updates from the update queue
  StakeSnapshotPtr next{};
  if (update_queue_.ApplyUpdates(current.body.block_number, current_, next))
  {
    // update the entry in the history
    stake_history_[current.body.block_number] = next;

    // the current stake snapshot has been replaced
    current_             = std::move(next);
    current_block_index_ = current.body.block_number;
  }

  TrimToSize(stake_history_, HISTORY_LENGTH);
}

StakeManager::CommitteePtr StakeManager::BuildCommittee(Block const &current)
{
  auto snapshot = LookupStakeSnapshot(current.body.block_number);
  return snapshot->BuildCommittee(current.body.entropy, committee_size_);
}

StakeManager::CommitteePtr StakeManager::Reset(StakeSnapshot const &snapshot)
{
  return ResetInternal(std::make_shared<StakeSnapshot>(snapshot));
}

StakeManager::CommitteePtr StakeManager::Reset(StakeSnapshot &&snapshot)
{
  return ResetInternal(std::make_shared<StakeSnapshot>(std::move(snapshot)));
}

StakeManager::CommitteePtr StakeManager::ResetInternal(StakeSnapshotPtr &&snapshot)
{
  // history
  stake_history_.clear();
  stake_history_[0] = snapshot;

  CommitteePtr new_committee = snapshot->BuildCommittee(0, committee_size_);

  // current
  current_             = std::move(snapshot);
  current_block_index_ = 0;

  return new_committee;
}

StakeManager::StakeSnapshotPtr StakeManager::LookupStakeSnapshot(BlockIndex block)
{
  // 9/10 time during normal operation the current stake snapshot will be used
  if (block >= current_block_index_)
  {
    return current_;
  }
  else
  {
    // on catchup, or in the case of multiple forks historical entries will be used
    auto upper_bound = stake_history_.upper_bound(block);

    if (upper_bound == stake_history_.begin())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Update to lookup stake snapshot for block ", block);
      return {};
    }
    else
    {
      // we are not interested in the upper bound, but the preceding historical element i.e.
      // the previous block change
      return (--upper_bound)->second;
    }
  }
}

}  // namespace ledger
}  // namespace fetch
