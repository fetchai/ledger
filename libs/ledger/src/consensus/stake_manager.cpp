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

StakeManager::StakeManager(EntropyGeneratorInterface &entropy)
  : entropy_{entropy}
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
  bool generate{false};

  auto const committee = GetCommittee(previous);
  if (!committee || committee->empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to determine committee for block generation");
  }
  else
  {
    // TODO(EJF): A reporting back mechanism will need to be added in order to handle the cases
    //            where blocks are not generated in a given time interval
    generate = (*committee)[0] == address;
  }

  return generate;
}

StakeManager::CommitteePtr StakeManager::GetCommittee(Block const &previous)
{
  assert(static_cast<bool>(current_));

  // generate the entropy for the previous block
  auto const entropy = entropy_.GenerateEntropy(previous.body.hash, previous.body.block_number);

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
