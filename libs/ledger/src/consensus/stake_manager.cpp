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

#include "core/random/lcg.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/consensus/entropy_generator_interface.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/stake_snapshot.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <random>

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "StakeMgr";
using DRNG                         = random::LinearCongruentialGenerator;

std::size_t SafeDecrement(std::size_t value, std::size_t decrement)
{
  if (decrement >= value)
  {
    return 0;
  }
  else
  {
    return value - decrement;
  }
}

template <typename T>
void TrimToSize(T &container, uint64_t max_allowed)
{
  if (container.size() >= max_allowed)
  {
    auto const num_to_remove = container.size() - max_allowed;

    if (num_to_remove > 0)
    {
      auto end = container.begin();
      std::advance(end, static_cast<std::ptrdiff_t>(num_to_remove));

      container.erase(container.begin(), end);
    }
  }
}

template <typename T>
void DeterministicShuffle(T &container, uint64_t entropy)
{
  DRNG rng(entropy);
  std::shuffle(container.begin(), container.end(), rng);
}

}  // namespace

StakeManager::StakeManager(uint64_t committee_size, uint32_t block_interval_ms,
                           uint64_t snapshot_validity_periodicity)
  : committee_size_{committee_size}
  , snapshot_validity_periodicity_{snapshot_validity_periodicity}
  , block_interval_ms_{block_interval_ms}
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

  // If this would create a new committee save this snapshot
  if (current.body.block_number % snapshot_validity_periodicity_ == 0)
  {
    auto snapshot = LookupStakeSnapshot(current.body.block_number);
    committee_history_[current.body.block_number] =
        snapshot->BuildCommittee(current.body.entropy, committee_size_);
  }

  TrimToSize(stake_history_, HISTORY_LENGTH);
  TrimToSize(committee_history_, HISTORY_LENGTH);
}

bool StakeManager::ShouldGenerateBlock(Block const &previous, Address const &address)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Should generate block? Prev: ", previous.body.block_number);

  auto const committee = GetCommittee(previous);

  if (!committee || committee->empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to determine committee for block generation");
    return false;
  }

  // At this point the miner will decide if they should produce a block. The first miner in the
  // committee will wait until block_interval after the block at the HEAD of the chain, the second
  // miner 2*block_interval and so on.
  uint32_t time_to_wait = block_interval_ms_;
  bool     in_committee = false;

  bool found = false;

  for (std::size_t i = 0; i < (*committee).size(); ++i)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Block: ", previous.body.block_number,
                    " Saw committee member: ", Address((*committee)[i]).address().ToBase64(),
                    "we are: ", address.address().ToBase64());

    if (Address((*committee)[i]) == address)
    {
      in_committee = true;
      found        = true;
    }

    if (!found)
    {
      time_to_wait += block_interval_ms_;
    }
  }

  uint64_t time_now_ms           = static_cast<uint64_t>(std::time(nullptr)) * 1000;
  uint64_t desired_time_for_next = (previous.first_seen_timestamp * 1000) + time_to_wait;

  if (in_committee && desired_time_for_next <= time_now_ms)
  {
    return true;
  }

  return false;
}

StakeManager::CommitteePtr StakeManager::GetCommittee(Block const &previous)
{
  // Calculate the last relevant snapshot
  uint64_t const last_snapshot =
      previous.body.block_number - (previous.body.block_number % snapshot_validity_periodicity_);

  // Invalid to request a committee too far ahead in time
  assert(committee_history_.find(last_snapshot) != committee_history_.end());

  if (committee_history_.find(last_snapshot) == committee_history_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "No committee history found for block: ", previous.body.block_number, " AKA ",
                   last_snapshot);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Periodicity: ", snapshot_validity_periodicity_);

  // If the last committee was the valid committee, return this. Otherwise, deterministically
  // shuffle the committee using the random beacon
  if (last_snapshot == previous.body.block_number)
  {
    return committee_history_.at(last_snapshot);
  }
  else
  {
    CommitteePtr committee_ptr = committee_history_[last_snapshot];
    assert(!committee_ptr->empty());

    Committee committee_copy = *committee_ptr;

    DeterministicShuffle(committee_copy, previous.body.entropy);

    return std::make_shared<Committee>(committee_copy);
  }
}

uint64_t StakeManager::GetBlockGenerationWeight(Block const &previous, Address const &address)
{
  auto const committee = GetCommittee(previous);

  if (!committee)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to determine block generation weight");
    return 0;
  }

  std::size_t weight{committee->size()};

  // TODO(EJF): Depending on the committee sizes this would need to be improved
  for (auto const &member : *committee)
  {
    if (address == Address(member))
    {
      break;
    }

    weight = SafeDecrement(weight, 1);
  }

  // Note: weight must always be non zero (indicates failure/not in committee)
  return weight;
}

void StakeManager::Reset(StakeSnapshot const &snapshot)
{
  ResetInternal(std::make_shared<StakeSnapshot>(snapshot));
}

void StakeManager::Reset(StakeSnapshot &&snapshot)
{
  ResetInternal(std::make_shared<StakeSnapshot>(std::move(snapshot)));
}

void StakeManager::ResetInternal(StakeSnapshotPtr &&snapshot)
{
  // history
  stake_history_.clear();
  stake_history_[0] = snapshot;

  committee_history_[0] = snapshot->BuildCommittee(0, committee_size_);

  if (committee_history_.find(0) == committee_history_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No committee history found for block when resetting.");
  }

  // current
  current_             = std::move(snapshot);
  current_block_index_ = 0;
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

bool StakeManager::ValidMinerForBlock(Block const &previous, Address const &address)
{
  auto const committee = GetCommittee(previous);

  if (!committee || committee->empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to determine committee for block validation");
    return false;
  }

  return std::find_if((*committee).begin(), (*committee).end(),
                      [&address](Identity const &identity) {
                        return address == Address(identity);
                      }) != (*committee).end();
}

uint32_t StakeManager::BlockInterval()
{
  return block_interval_ms_;
}

}  // namespace ledger
}  // namespace fetch
