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
namespace {

constexpr char const *LOGGING_NAME = "StakeMgr";

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

}  // namespace

StakeManager::StakeManager(EntropyGeneratorInterface &entropy, uint32_t block_interval_ms)
  : entropy_{&entropy}
  , block_interval_ms_{block_interval_ms}
{}

void StakeManager::UpdateCurrentBlock(Block const &current)
{
  // need to evaluate any of the updates from the update queue
  StakeSnapshotPtr next{};
  if (update_queue_.ApplyUpdates(current.body.block_number, current_, next))
  {
    // update the entry in the history
    history_[current.body.block_number] = next;

    // the current stake snapshot has been replaced
    current_             = std::move(next);
    current_block_index_ = current.body.block_number;
  }
}

bool StakeManager::ShouldGenerateBlock(Block const &previous, Address const &address)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Should generate block? Prev: ", previous.body.block_number);

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

  for (std::size_t i = 0; i < (*committee).size(); ++i)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Saw committee member: ", (*committee)[i].address().ToBase64(),
                   "we are: ", address.address().ToBase64());

    if ((*committee)[i] == address)
    {
      in_committee = true;
      break;
    }
    time_to_wait += block_interval_ms_;
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
  assert(static_cast<bool>(current_));

  // generate the entropy for the previous block
  uint64_t entropy{0};
  if (!LookupEntropy(previous, entropy))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup committee for ", previous.body.block_number,
                   " (entropy not ready)");
    return {};
  }

  // lookup the snapshot associated
  auto snapshot = LookupStakeSnapshot(previous.body.block_number);
  if (!snapshot)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup committee for ", previous.body.block_number);
    return {};
  }

  // TODO(EJF): Committees can be directly cached
  return snapshot->BuildCommittee(entropy, committee_size_);
}

std::size_t StakeManager::GetBlockGenerationWeight(Block const &previous, Address const &address)
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
    if (address == member)
    {
      break;
    }

    weight = SafeDecrement(weight, 1);
  }

  // Note: weight must always be non zero (indicates failure/not in committee)
  return weight;
}

void StakeManager::Reset(StakeSnapshot const &snapshot, std::size_t committee_size)
{
  ResetInternal(std::make_shared<StakeSnapshot>(snapshot), committee_size);
}

void StakeManager::Reset(StakeSnapshot &&snapshot, std::size_t committee_size)
{
  ResetInternal(std::make_shared<StakeSnapshot>(std::move(snapshot)), committee_size);
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
    auto upper_bound = history_.upper_bound(block);

    if (upper_bound == history_.begin())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Update to lookup stake snapshot for block ", block);
      return {};
    }
    else
    {
      // we are not interested in the upper bound, but the preceeding historical elemeent i.e.
      // the previous block change
      return (--upper_bound)->second;
    }
  }
}

void StakeManager::ResetInternal(StakeSnapshotPtr &&snapshot, std::size_t committee_size)
{
  // config
  committee_size_ = committee_size;

  // history
  history_.clear();
  history_[0] = snapshot;

  // current
  current_             = std::move(snapshot);
  current_block_index_ = 0;
}

bool StakeManager::LookupEntropy(Block const &previous, uint64_t &entropy)
{
  bool success{false};

  // Step 1. Lookup the entropy
  auto const it = entropy_cache_.find(previous.body.block_number);
  if (entropy_cache_.end() != it)
  {
    entropy = it->second;
    success = true;
  }
  else
  {
    // generate the entropy for the previous block
    auto const status =
        entropy_->GenerateEntropy(previous.body.hash, previous.body.block_number, entropy);

    if (EntropyGeneratorInterface::Status::OK == status)
    {
      entropy_cache_[previous.body.block_number] = entropy;
      success                                    = true;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup entropy for block ",
                     previous.body.block_number);
    }
  }

  // Step 2. Clean up
  if (entropy_cache_.size() >= HISTORY_LENGTH)
  {
    auto const num_to_remove = entropy_cache_.size() - HISTORY_LENGTH;

    if (num_to_remove > 0)
    {
      auto end = entropy_cache_.begin();
      std::advance(end, static_cast<std::ptrdiff_t>(num_to_remove));

      entropy_cache_.erase(entropy_cache_.begin(), end);
    }
  }

  return success;
}

bool StakeManager::ValidMinerForBlock(Block const &previous, Address const &address)
{
  auto const committee = GetCommittee(previous);

  if (!committee || committee->empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to determine committee for block validation");
    return false;
  }

  return std::find((*committee).begin(), (*committee).end(), address) != (*committee).end();
}

}  // namespace ledger
}  // namespace fetch
