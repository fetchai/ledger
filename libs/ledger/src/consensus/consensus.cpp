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

#include <ctime>
#include <random>
#include <utility>

#include "ledger/consensus/consensus.hpp"

constexpr char const *LOGGING_NAME = "Consensus";

using Consensus       = fetch::ledger::Consensus;
using NextBlockPtr    = Consensus::NextBlockPtr;
using Status          = Consensus::Status;
using StakeManagerPtr = Consensus::StakeManagerPtr;

namespace {
using DRNG = fetch::random::LinearCongruentialGenerator;

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
void DeterministicShuffle(T &container, uint64_t entropy)
{
  DRNG rng(entropy);
  std::shuffle(container.begin(), container.end(), rng);
}
}  // namespace

Consensus::Consensus(StakeManagerPtr stake, BeaconServicePtr beacon, MainChain const &chain,
                     Identity mining_identity, uint64_t aeon_period, uint64_t max_committee_size,
                     uint32_t block_interval_ms)
  : stake_{std::move(stake)}
  , beacon_{std::move(beacon)}
  , chain_{chain}
  , mining_identity_{std::move(mining_identity)}
  , mining_address_{Address(mining_identity_)}
  , aeon_period_{aeon_period}
  , max_committee_size_{max_committee_size}
  , block_interval_ms_{block_interval_ms}
{
  assert(stake_);
  FETCH_UNUSED(chain_);
}

Consensus::CommitteePtr Consensus::GetCommittee(Block const &previous)
{
  // Calculate the last relevant snapshot
  uint64_t const last_snapshot =
      previous.body.block_number - (previous.body.block_number % aeon_period_);

  // Invalid to request a committee too far ahead in time
  assert(committee_history_.find(last_snapshot) != committee_history_.end());

  if (committee_history_.find(last_snapshot) == committee_history_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "No committee history found for block: ", previous.body.block_number, " AKA ",
                   last_snapshot);
    return nullptr;
  }

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

bool Consensus::ValidMinerForBlock(Block const &previous, Address const &address)
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

uint64_t Consensus::GetBlockGenerationWeight(Block const &previous, Address const &address)
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

bool Consensus::ShouldGenerateBlock(Block const &previous, Address const &address)
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

void Consensus::UpdateCurrentBlock(Block const &current)
{
  if (current.body.block_number > current_block_number_ &&
      current.body.block_number != current_block_number_ + 1)
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "Updating the current block more than one block ahead is invalid");
    return;
  }

  // temporary behaviour - do not allow updating backwards
  if (current.body.block_number < current_block_number_)
  {
    return;
  }

  const bool is_genesis = current.body.block_number == 0;

  current_block_        = current;
  current_block_number_ = current.body.block_number;

  if (!is_genesis)
  {
    stake_->UpdateCurrentBlock(current_block_);
  }

  if ((current_block_number_ % aeon_period_) == 0 &&
      int64_t(current_block_number_) > last_committee_created_)
  {
    last_committee_created_ = int64_t(current_block_number_);

    CabinetMemberList cabinet_member_list;
    committee_history_[current.body.block_number] = stake_->BuildCommittee(current_block_);

    TrimToSize(committee_history_, HISTORY_LENGTH);

    for (auto const &staker : *committee_history_[current.body.block_number])
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Adding staker: ", staker.identifier().ToBase64());
      cabinet_member_list.insert(staker.identifier());
    }

    uint32_t threshold = static_cast<uint32_t>(
        std::ceil(static_cast<double>(cabinet_member_list.size()) * threshold_));

    FETCH_LOG_INFO(LOGGING_NAME, "Block: ", current_block_number_,
                   " creating new aeon. Periodicity: ", aeon_period_, " threshold: ", threshold,
                   " as double: ", threshold_, " cabinet size: ", cabinet_member_list.size());

    uint64_t last_block_time = current.body.timestamp;
    uint64_t current_time    = static_cast<uint64_t>(std::time(nullptr));

    if (current.body.block_number == 0)
    {
      last_block_time = default_start_time_;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Starting DKG with timestamp: ", last_block_time,
                   " current: ", current_time,
                   " diff: ", int64_t(current_time) - int64_t(last_block_time));

    uint64_t block_interval = 1;

    beacon_->StartNewCabinet(cabinet_member_list, threshold, current_block_number_,
                             current_block_number_ + aeon_period_ + 1,
                             last_block_time + block_interval);
  }
  else if (!(current_block_number_ % aeon_period_ == 0))
  {
    beacon_->AbortCabinet(current_block_number_);
  }
}

NextBlockPtr Consensus::GenerateNextBlock()
{
  NextBlockPtr ret;

  // Number of block we want to generate
  uint64_t const block_number = current_block_number_ + 1;
  uint64_t       entropy      = 0;

  // Try to get entropy for the block we are generating - is allowed to fail if we request too
  // early
  if (EntropyGeneratorInterface::Status::OK !=
      beacon_->GenerateEntropy(current_block_.body.hash, block_number, entropy))
  {
    return ret;
  }

  // Note here the previous block's entropy determines miner selection
  if (!ShouldGenerateBlock(current_block_, mining_address_))
  {
    return ret;
  }

  ret = std::make_unique<Block>();

  ret->body.previous_hash = current_block_.body.hash;
  ret->body.block_number  = block_number;
  ret->body.miner         = mining_address_;
  ret->body.entropy       = entropy;
  ret->weight             = GetBlockGenerationWeight(current_block_, mining_address_);

  return ret;
}

Status Consensus::ValidBlock(Block const &previous, Block const &current)
{
  Status ret = Status::YES;

  // Attempt to ascertain the beacon value within the block
  uint64_t entropy = 0;

  // Try to get entropy for the block we are generating - because of races it could be
  // we have yet to receive it from the network.
  auto result = beacon_->GenerateEntropy(current.body.hash, current.body.block_number, entropy);

  switch (result)
  {
  case EntropyGeneratorInterface::Status::NOT_READY:
    FETCH_LOG_INFO(LOGGING_NAME, "Too early for entropy for block: ", current.body.block_number);
    ret = Status::UNKNOWN;
    break;
  case EntropyGeneratorInterface::Status::OK:
    if (entropy == current.body.entropy)
    {
      ret = Status::YES;
    }
    else
    {
      ret = Status::NO;
    }
    break;
  case EntropyGeneratorInterface::Status::FAILED:
    ret = Status::NO;
    break;
  }

  if (ret == Status::NO)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Saw incorrect random beacon from: ",
                   current_block_.body.miner.address().ToBase64(),
                   ".Block number: ", current_block_.body.block_number, " expected: ", entropy,
                   " got: ", current_block_.body.entropy);
    return Status::NO;
  }

  // Here we assume the entropy is correct and proceed to verify against the stake subsystem
  if (!ValidMinerForBlock(previous, current.body.miner))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Received block has incorrect miner identity set");
    return Status::NO;
  }

  if (current.weight != GetBlockGenerationWeight(previous, current.body.miner) &&
      current.weight != 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Incorrect stake weight found for block. Weight: ", current.weight);
    return Status::NO;
  }

  return Status::YES;
}

void Consensus::Reset(StakeSnapshot const &snapshot)
{
  committee_history_[0] = stake_->Reset(snapshot);

  if (committee_history_.find(0) == committee_history_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No committee history found for block when resetting.");
  }
}

void Consensus::Refresh()
{}

void Consensus::SetThreshold(double threshold)
{
  threshold_ = threshold;
  FETCH_LOG_INFO(LOGGING_NAME, "Set threshold to ", threshold_);
}

void Consensus::SetCommitteeSize(uint64_t size)
{
  max_committee_size_ = size;
  stake_->SetCommitteeSize(max_committee_size_);
}

StakeManagerPtr Consensus::stake()
{
  return stake_;
}

void Consensus::SetDefaultStartTime(uint64_t default_start_time)
{
  default_start_time_ = default_start_time;
}
