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

#include "ledger/consensus/stake_manager.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "logging/logging.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "storage/resource_mapper.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "StakeMgr";

storage::ResourceAddress const STORAGE_ADDRESS{"fetch.token.state.aggregation.stake"};

} // namespace

void StakeManager::UpdateCurrentBlock(BlockIndex block_number)
{
  // this should never be called for the genesis block
  assert(block_number != 0);

  // need to evaluate any of the updates from the update queue
  StakeSnapshotPtr next{};
  if (update_queue_.ApplyUpdates(block_number, current_, next))
  {
    // update the entry in the history
    stake_history_[block_number] = next;

    // the current stake snapshot has been replaced
    current_             = std::move(next);
    current_block_index_ = block_number;
  }

  TrimToSize(stake_history_, HISTORY_LENGTH);
}

StakeManager::CommitteePtr StakeManager::BuildCommittee(Block const &current,
                                                        uint64_t     committee_size)
{
  return BuildCommittee(current.body.block_number, current.body.block_entropy.EntropyAsU64(),
                        committee_size);
}

StakeManager::CommitteePtr StakeManager::BuildCommittee(uint64_t block_number, uint64_t entropy,
                                                        uint64_t committee_size) const
{
  auto snapshot = LookupStakeSnapshot(block_number);
  return snapshot->BuildCommittee(entropy, committee_size);
}

bool StakeManager::Save(StorageInterface &storage)
{
  bool success{false};

  try
  {
    serializers::LargeObjectSerializeHelper serializer{};
    serializer << *this;

    storage.Set(STORAGE_ADDRESS, serializer.data());

    success = true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to save stake manager to storage: ", ex.what());
  }

  return success;
}

bool StakeManager::Load(StorageInterface &storage)
{
  bool success{false};

  try
  {
    auto const result = storage.Get(STORAGE_ADDRESS);

    if (!result.document.empty())
    {
      serializers::LargeObjectSerializeHelper serializer{result.document};
      serializer >> *this;
    }

    success = true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to load stake manager from storage: ", ex.what());
  }

  return success;
}

StakeManager::CommitteePtr StakeManager::Reset(StakeSnapshot const &snapshot, uint64_t committee_size)
{
  return ResetInternal(std::make_shared<StakeSnapshot>(snapshot), committee_size);
}

StakeManager::CommitteePtr StakeManager::Reset(StakeSnapshot &&snapshot, uint64_t committee_size)
{
  return ResetInternal(std::make_shared<StakeSnapshot>(std::move(snapshot)), committee_size);
}

StakeManager::CommitteePtr StakeManager::ResetInternal(StakeSnapshotPtr &&snapshot, uint64_t committee_size)
{
  // history
  stake_history_.clear();
  stake_history_[0] = snapshot;

  CommitteePtr new_committee = snapshot->BuildCommittee(0, committee_size);

  // current
  current_             = std::move(snapshot);
  current_block_index_ = 0;

  return new_committee;
}

StakeManager::StakeSnapshotPtr StakeManager::LookupStakeSnapshot(BlockIndex block) const
{
  // 9/10 time during normal operation the current stake snapshot will be used
  if (block >= current_block_index_)
  {
    return current_;
  }

  // on catchup, or in the case of multiple forks historical entries will be used
  auto upper_bound = stake_history_.upper_bound(block);

  if (upper_bound == stake_history_.begin())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Update to lookup stake snapshot for block ", block);
    return {};
  }

  // we are not interested in the upper bound, but the preceding historical element i.e.
  // the previous block change
  return (--upper_bound)->second;
}

}  // namespace ledger
}  // namespace fetch
