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

#include "ledger/chain/block.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "logging/logging.hpp"
#include "storage/resource_mapper.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "StakeMgr";

storage::ResourceAddress const STAKE_STORAGE_ADDRESS{"fetch.token.state.aggregation.stake"};

}  // namespace

void StakeManager::UpdateCurrentBlock(BlockIndex block_index)
{
  // The first stake can only be set through a reset event
  if (block_index == 0)
  {
    return;
  }

  // need to evaluate any of the updates from the update queue
  StakeSnapshotPtr next{};
  if (update_queue_.ApplyUpdates(block_index, current_, next))
  {
    // update the entry in the history
    stake_history_[block_index] = next;

    // the current stake snapshot has been replaced
    current_             = std::move(next);
    current_block_index_ = block_index;
  }

  TrimToSize(stake_history_, HISTORY_LENGTH);
}

StakeManager::CabinetPtr StakeManager::BuildCabinet(
    Block const &current, uint64_t cabinet_size,
    ConsensusInterface::Minerwhitelist const &whitelist)
{
  CabinetPtr cabinet{};

  auto snapshot = LookupStakeSnapshot(current.block_number);
  if (snapshot)
  {
    cabinet = snapshot->BuildCabinet(current.block_entropy.EntropyAsU64(), cabinet_size, whitelist);
  }

  return cabinet;
}

StakeManager::CabinetPtr StakeManager::BuildCabinet(
    uint64_t block_number, uint64_t entropy, uint64_t cabinet_size,
    ConsensusInterface::Minerwhitelist const &whitelist) const
{
  auto snapshot = LookupStakeSnapshot(block_number);
  return snapshot->BuildCabinet(entropy, cabinet_size, whitelist);
}

bool StakeManager::Save(StorageInterface &storage)
{
  bool success{false};

  try
  {
    serializers::LargeObjectSerializeHelper serializer{};
    serializer << *this;

    storage.Set(STAKE_STORAGE_ADDRESS, serializer.data());

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
    auto const result = storage.Get(STAKE_STORAGE_ADDRESS);

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

StakeManager::CabinetPtr StakeManager::Reset(StakeSnapshot const &snapshot, uint64_t cabinet_size)
{
  return ResetInternal(std::make_shared<StakeSnapshot>(snapshot), cabinet_size);
}

StakeManager::CabinetPtr StakeManager::Reset(StakeSnapshot &&snapshot, uint64_t cabinet_size)
{
  return ResetInternal(std::make_shared<StakeSnapshot>(std::move(snapshot)), cabinet_size);
}

StakeManager::CabinetPtr StakeManager::ResetInternal(StakeSnapshotPtr &&snapshot,
                                                     uint64_t           cabinet_size)
{
  // history
  stake_history_.clear();
  stake_history_[0] = snapshot;

  CabinetPtr new_cabinet = snapshot->BuildCabinet(0, cabinet_size);

  // current
  current_             = std::move(snapshot);
  current_block_index_ = 0;

  return new_cabinet;
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
    FETCH_LOG_WARN(LOGGING_NAME, "Update to look up stake snapshot for block ", block);
    return {};
  }

  // we are not interested in the upper bound, but the preceding historical element i.e.
  // the previous block change
  return (--upper_bound)->second;
}

}  // namespace ledger
}  // namespace fetch
