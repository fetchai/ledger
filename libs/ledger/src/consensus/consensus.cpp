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

#include <utility>

#include "ledger/consensus/consensus.hpp"

constexpr char const *LOGGING_NAME = "Consensus";

using Consensus       = fetch::ledger::Consensus;
using NextBlockPtr    = Consensus::NextBlockPtr;
using Status          = Consensus::Status;
using StakeManagerPtr = Consensus::StakeManagerPtr;

Consensus::Consensus(StakeManagerPtr stake, BeaconServicePtr beacon, MainChain const &chain,
                     Identity mining_identity, uint64_t aeon_period, uint64_t max_committee_size)
  : stake_{std::move(stake)}
  , beacon_{std::move(beacon)}
  , chain_{chain}
  , mining_identity_{std::move(mining_identity)}
  , mining_address_{Address(mining_identity_)}
  , aeon_period_{aeon_period}
  , max_committee_size_{max_committee_size}
{
  assert(stake_);
  FETCH_UNUSED(chain_);
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
    auto const        current_stakers = stake_->GetCommittee(current_block_);

    for (auto const &staker : *current_stakers)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Adding staker: ", staker.identifier().ToBase64());
      cabinet_member_list.insert(staker);
    }

    uint32_t threshold = static_cast<uint32_t>(
        std::ceil(static_cast<double>(cabinet_member_list.size()) * threshold_));

    FETCH_LOG_INFO(LOGGING_NAME, "Block: ", current_block_number_,
                   " creating new aeon. Periodicity: ", aeon_period_, " threshold: ", threshold,
                   " as double: ", threshold_, " cabinet size: ", cabinet_member_list.size());

    beacon_->StartNewCabinet(cabinet_member_list, threshold, current_block_number_,
                             current_block_number_ + aeon_period_ + 1);
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
  if (!stake_->ShouldGenerateBlock(current_block_, mining_address_))
  {
    return ret;
  }

  ret = std::make_unique<Block>();

  ret->body.previous_hash = current_block_.body.hash;
  ret->body.block_number  = block_number;
  ret->body.miner         = mining_address_;
  ret->body.entropy       = entropy;
  ret->weight             = stake_->GetBlockGenerationWeight(current_block_, mining_address_);

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
  if (!stake_->ValidMinerForBlock(previous, current.body.miner))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Received block has incorrect miner identity set");
    return Status::NO;
  }

  if (current.weight != stake_->GetBlockGenerationWeight(previous, current.body.miner) &&
      current.weight != 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Incorrect stake weight found for block. Weight: ", current.weight);
    return Status::NO;
  }

  return Status::YES;
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
