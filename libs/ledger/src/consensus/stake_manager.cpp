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
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/entropy_generator_interface.hpp"

#include <algorithm>

namespace fetch {
namespace ledger {
namespace {

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

} // namespace

StakeManager::StakeManager(EntropyGeneratorInterface &entropy)
  : entropy_{entropy}
{
}

StakeManager::Validity StakeManager::Validate(Block const &block) const
{
  return Validity::INDETERMINATE;
}

void StakeManager::UpdateCurrentBlock(Block const &current)
{
  // need to evaluate any of the updates from the update queue
  StakeSnapshotPtr next{};
  if (update_queue_.ApplyUpdates(current.body.block_number, current_, next))
  {
    // update the entry in the history
    history_[current.body.block_number] = next;

    // the current stake snapshot has been replaced
    current_ = std::move(next);
  }
}

bool StakeManager::ShouldGenerateBlock(Block const &previous)
{
  return false;
}

StakeManager::Committee const &StakeManager::GetCommittee(Block const &previous)
{
  assert(static_cast<bool>(current_));

  auto const entropy = entropy_.GenerateEntropy(previous.body.hash, previous.body.block_number);

  cached_committee_ = current_->BuildCommittee(entropy, committee_size_);

  return cached_committee_;
}

std::size_t StakeManager::GetBlockGenerationWeight(Block const &previous, Address const &address)
{
  auto const &committee = GetCommittee(previous);

  std::size_t weight{committee.size()};

  // TODO(EJF): Depending on the committee sizes this would need to be improved
  for (auto const &member : committee)
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

void StakeManager::ResetInternal(StakeSnapshotPtr &&snapshot, std::size_t committee_size)
{
  committee_size_ = committee_size;
  history_.clear();
  history_[0] = snapshot;
  current_ = std::move(snapshot);
}

} // namespace ledger
} // namespace fetch
