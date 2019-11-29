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

#include "beacon/block_entropy.hpp"
#include "core/random/lcg.hpp"
#include "ledger/consensus/consensus.hpp"

#include <ctime>
#include <random>
#include <utility>

/**
 * Consensus enforcement class.
 *
 * During operation, miners will stake tokens in order to get into
 * the staking pool and are known as stakers (at genesis
 * certain stakers will be pre-set). These stakers
 * are selected to form a 'cabinet'.
 *
 * The cabinet will form a 'group public key' through
 * use of threshold cryptography (MCL scheme). This is referred to
 * as 'distributed key generation', DKG.
 *
 * Of these N cabinet members, during DKG some subset M will qualify to be able to
 * sign the group public key. These are known as 'the qualified set',
 * or qual.
 *
 * During block production, entropy is generated in blocks by signing
 * (using this group key) the previous group signature. This entropy
 * then sets on a block by block basis the priority ranking of qual.
 *
 * In the ideal case, the first member of qual for that block/entropy
 * will produce the heaviest block to be accepted by the network. Other
 * members of qual are forced to wait relative to block time to make
 * a competing fork, for efficiency reasons.
 *
 * There are a number of thresholds/system limits:
 * - max cabinet size        , the maximum allowed cabinet taken from stakers
 * - qual threshold          , qual must be no less than 2/3rds of the cabinet
 * - signing threshold       , (1/2)+1 of the cabinet
 * - confirmation threshold  , set to signing threshold
 */

namespace {

constexpr char const *LOGGING_NAME = "Consensus";

using Consensus       = fetch::ledger::Consensus;
using NextBlockPtr    = Consensus::NextBlockPtr;
using Status          = Consensus::Status;
using StakeManagerPtr = Consensus::StakeManagerPtr;
using BlockPtr        = Consensus::BlockPtr;

using fetch::ledger::MainChain;
using fetch::ledger::Block;

using DRNG = fetch::random::LinearCongruentialGenerator;

std::size_t SafeDecrement(std::size_t value, std::size_t decrement)
{
  if (decrement >= value)
  {
    return 0;
  }

  return value - decrement;
}

template <typename T>
T DeterministicShuffle(T &container, uint64_t entropy)
{
  DRNG rng(entropy);
  std::shuffle(container.begin(), container.end(), rng);
  return container;
}

}  // namespace

Consensus::Consensus(StakeManagerPtr stake, BeaconSetupServicePtr beacon_setup,
                     BeaconServicePtr beacon, MainChain const &chain, StorageInterface &storage,
                     Identity mining_identity, uint64_t aeon_period, uint64_t max_cabinet_size,
                     uint64_t block_interval_ms, NotarisationPtr notarisation)
  : storage_{storage}
  , stake_{std::move(stake)}
  , cabinet_creator_{std::move(beacon_setup)}
  , beacon_{std::move(beacon)}
  , chain_{chain}
  , mining_identity_{std::move(mining_identity)}
  , mining_address_{chain::Address(mining_identity_)}
  , aeon_period_{aeon_period}
  , max_cabinet_size_{max_cabinet_size}
  , block_interval_ms_{block_interval_ms}
  , notarisation_{std::move(notarisation)}
{
  assert(stake_);
}

// TODO(HUT): probably this is not required any more.
Consensus::CabinetPtr Consensus::GetCabinet(Block const &previous) const
{
  // Calculate the last relevant snapshot
  uint64_t const last_snapshot = previous.block_number - (previous.block_number % aeon_period_);

  // Invalid to request a cabinet too far ahead in time
  assert(cabinet_history_.find(last_snapshot) != cabinet_history_.end());

  if (cabinet_history_.find(last_snapshot) == cabinet_history_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No cabinet history found for block: ", previous.block_number,
                   " AKA ", last_snapshot);
    return nullptr;
  }

  // If the last cabinet was the valid cabinet, return this. Otherwise, deterministically
  // shuffle the cabinet using the random beacon
  if (last_snapshot == previous.block_number)
  {
    return cabinet_history_.at(last_snapshot);
  }

  auto cabinet_ptr = cabinet_history_.find(last_snapshot);
  assert(cabinet_ptr != cabinet_history_.end());

  Cabinet cabinet_copy = *(cabinet_ptr->second);

  DeterministicShuffle(cabinet_copy, previous.block_entropy.EntropyAsU64());

  return std::make_shared<Cabinet>(cabinet_copy);
}

uint32_t Consensus::GetThreshold(Block const &block) const
{
  auto cabinet_size = static_cast<double>(GetCabinet(block)->size());
  return static_cast<uint32_t>(std::floor(cabinet_size * threshold_)) + 1;
}

BlockPtr GetBlockPriorTo(Block const &current, MainChain const &chain)
{
  return chain.GetBlock(current.previous_hash);
}

Block GetBeginningOfAeon(Block const &current, MainChain const &chain)
{
  Block ret = current;

  // Walk back the chain until we see a block specifying an aeon beginning (corner
  // case for true genesis)
  while (!ret.block_entropy.IsAeonBeginning() && current.block_number != 0)
  {
    auto prior = GetBlockPriorTo(ret, chain);

    if (!prior)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Failed to find the beginning of the aeon when traversing the chain!");
      throw std::runtime_error("Failed to traverse main chain");
    }

    ret = *prior;
  }

  return ret;
}

bool Consensus::VerifyNotarisation(Block const &block) const
{
  // Genesis is not notarised so the body of blocks with block number 1 do
  // not contain a notarisation
  if (notarisation_ && block.block_number > 1)
  {
    // Try to verify with notarisation units in notarisation_
    auto previous_notarised_block = chain_.GetBlock(block.previous_hash);

    if (!previous_notarised_block)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to find preceding block when verifying notarisation!");
      return false;
    }

    auto result = notarisation_->Verify(previous_notarised_block->block_number,
                                        previous_notarised_block->hash,
                                        block.block_entropy.block_notarisation);
    if (result == NotarisationResult::CAN_NOT_VERIFY)
    {
      // If block is too old then get aeon beginning
      auto aeon_block                = GetBeginningOfAeon(*previous_notarised_block, chain_);
      auto threshold                 = GetThreshold(*previous_notarised_block);
      auto ordered_notarisation_keys = aeon_block.block_entropy.aeon_notarisation_keys;

      return notarisation_->Verify(previous_notarised_block->hash,
                                   block.block_entropy.block_notarisation,
                                   ordered_notarisation_keys, threshold);
    }
    if (result == NotarisationResult::FAIL_VERIFICATION)
    {
      return false;
    }
  }
  return true;
}

uint64_t Consensus::GetBlockGenerationWeight(Block const &previous, chain::Address const &address)
{
  auto beginning_of_aeon = GetBeginningOfAeon(previous, chain_);
  auto cabinet           = beginning_of_aeon.block_entropy.qualified;

  std::size_t weight{cabinet.size()};

  // TODO(EJF): Depending on the cabinet sizes this would need to be improved
  for (auto const &member : cabinet)
  {
    if (address == chain::Address::FromMuddleAddress(member))
    {
      break;
    }

    weight = SafeDecrement(weight, 1);
  }

  // Note: weight must always be non zero (indicates failure/not in cabinet)
  return weight;
}

Consensus::WeightedQual QualWeightedByEntropy(Consensus::BlockEntropy::Cabinet const &cabinet,
                                              uint64_t                                entropy)
{
  Consensus::WeightedQual ret;
  ret.reserve(cabinet.size());

  for (auto const &i : cabinet)
  {
    ret.emplace_back(i);
  }

  return DeterministicShuffle(ret, entropy);
}

/**
 * Determine whether the proposed block is valid according to consensus timing requirements.
 * Requirements for this are that the miner is a member of qual, and that it is within its rights
 * according to the time slot protocol and its weighting. So it tests:
 *
 * - block made by member of qual
 * -
 */
bool Consensus::ValidBlockTiming(Block const &previous, Block const &proposed) const
{
  FETCH_LOG_TRACE(LOGGING_NAME, "Should generate block? Prev: ", previous.block_number);

  Identity const &identity = proposed.miner_id;

  // Have to use the proposed block for this fn in case the block would be a new
  // aeon beginning.
  Block beginning_of_aeon = GetBeginningOfAeon(proposed, chain_);

  BlockEntropy::Cabinet qualified_cabinet = beginning_of_aeon.block_entropy.qualified;
  auto                  qualified_cabinet_weighted =
      QualWeightedByEntropy(qualified_cabinet, previous.block_entropy.EntropyAsU64());

  if (qualified_cabinet.find(identity.identifier()) == qualified_cabinet.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Miner ", identity.identifier().ToBase64(),
                   " attempted to mine block ", previous.block_number + 1,
                   " but was not part of qual:");

    for (auto const &member : qualified_cabinet)
    {
      FETCH_LOG_INFO(LOGGING_NAME, member.ToBase64());
    }

    return false;
  }

  // TODO(HUT): check block has been signed correctly

  // Time slot protocol: within the block period, only the heaviest weighted miner may produce a
  // block, outside this interval, any miner may produce a block.
  uint64_t last_block_timestamp_ms     = previous.timestamp * 1000;
  uint64_t proposed_block_timestamp_ms = proposed.timestamp * 1000;
  uint64_t time_now_ms =
      GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) * 1000;

  // Blocks must be ahead in time of the previous, and less than or equal to current time or they
  // will be rejected.
  if (proposed_block_timestamp_ms > time_now_ms)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Found block that appears to be minted ahead in time. This is invalid.");
    return false;
  }

  if (proposed_block_timestamp_ms < last_block_timestamp_ms)
  {
    FETCH_LOG_WARN(
        LOGGING_NAME,
        "Found block that indicates it was minted before the previous. This is invalid.");
    return false;
  }

  uint64_t const previous_block_window_ends = last_block_timestamp_ms + block_interval_ms_;

  // Blocks cannot be created within the block interval of the previous, this enforces
  // the block period
  if (proposed_block_timestamp_ms < previous_block_window_ends)
  {
    return false;
  }

  assert(!qualified_cabinet_weighted.empty());

  // Miners must additionally wait N block periods according to their rank (0 being the best)
  // Note, this is the identity specified in the block.
  auto const miner_rank = std::distance(
      qualified_cabinet_weighted.begin(),
      std::find(qualified_cabinet_weighted.begin(), qualified_cabinet_weighted.end(), identity));

  if (proposed_block_timestamp_ms >
      uint64_t(previous_block_window_ends + (uint64_t(miner_rank) * block_interval_ms_)))
  {
    if (identity == mining_identity_)
    {
      FETCH_LOG_DEBUG(
          LOGGING_NAME, "Minting block. Time now: ", time_now_ms,
          " Timestamp: ", block_interval_ms_, " proposed: ", proposed_block_timestamp_ms,
          " Prev window ends: ", previous_block_window_ends,
          " last block TS:  ", last_block_timestamp_ms, " miner rank: ", miner_rank, " target: ",
          uint64_t(previous_block_window_ends + (uint64_t(miner_rank) * block_interval_ms_)),
          " block weight: ", proposed.weight, " ident: ", mining_identity_.identifier().ToBase64());
    }
    return true;
  }

  return false;
}

/**
 * Whether the cabinet should be triggered
 */
bool ShouldTriggerAeon(uint64_t block_number, uint64_t aeon_period)
{
  return (block_number % aeon_period) == 0;
}

/**
 * Trigger a new cabinet on a trigger point, so long as the exact cabinet
 * wasn't already triggered. This will allow alternating cabinets to be triggered for the
 * same block height.
 *
 */
bool Consensus::ShouldTriggerNewCabinet(Block const &block)
{
  bool const trigger_point = ShouldTriggerAeon(block.block_number, aeon_period_);

  if (last_triggered_cabinet_ != block.hash && trigger_point)
  {
    last_triggered_cabinet_ = block.hash;
    return true;
  }

  return false;
}

void Consensus::UpdateCurrentBlock(Block const &current)
{
  bool const one_ahead = current.block_number == current_block_.block_number + 1;

  if (current.block_number > current_block_.block_number && !one_ahead)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Note: updating consensus with a block more than one ahead than last updated "
                   "block! Current: ",
                   current_block_.block_number, " Attempt: ", current.block_number);
  }

  // Don't try to set previous when we see genesis!
  if (current.block_number == 0)
  {
    current_block_ = current;
  }
  else
  {
    current_block_ = current;

    auto prior = GetBlockPriorTo(current_block_, chain_);

    if (!prior)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Failed to find the beginning of the aeon when updating the block!");
      throw std::runtime_error("Failed to update block!");
    }

    previous_block_    = *prior;
    beginning_of_aeon_ = GetBeginningOfAeon(current_block_, chain_);
  }

  if (!stake_->Load(storage_))
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "Failure to load stake information. block: ", current_block_.block_number);
    return;
  }

  // this might cause a trim that is not flushed to the state DB, however, on the next block the
  // full trim will take place and the state DB will be updated.
  stake_->UpdateCurrentBlock(current_block_.block_number);

  // Notify notarisation of new valid block
  if (notarisation_)
  {
    notarisation_->NotariseBlock(current_block_);
    if (current.block_entropy.IsAeonBeginning())
    {
      uint64_t round_start = current_block_.block_number;
      assert(!current_block_.block_entropy.aeon_notarisation_keys.empty());
      notarisation_->SetAeonDetails(round_start, round_start + aeon_period_ - 1,
                                    GetThreshold(current_block_),
                                    current_block_.block_entropy.aeon_notarisation_keys);
    }
  }

  if (ShouldTriggerNewCabinet(current_block_))
  {
    // attempt to build the cabinet from
    auto cabinet = stake_->BuildCabinet(current_block_, max_cabinet_size_);
    if (!cabinet)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Failed to build cabinet for block: ", current_block_.block_number);
      return;
    }

    CabinetMemberList cabinet_member_list;
    cabinet_history_[current.block_number] = cabinet;

    TrimToSize(cabinet_history_, HISTORY_LENGTH);

    bool member_of_cabinet{false};
    for (auto const &staker : *cabinet_history_[current.block_number])
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Adding staker: ", staker.identifier().ToBase64());

      if (staker == mining_identity_)
      {
        member_of_cabinet = true;
      }

      cabinet_member_list.insert(staker.identifier());
    }

    if (member_of_cabinet)
    {
      auto threshold =
          static_cast<uint32_t>(std::floor(static_cast<double>(cabinet_member_list.size())) *
                                threshold_) +
          1;

      FETCH_LOG_INFO(LOGGING_NAME, "Block: ", current_block_.block_number,
                     " creating new aeon. Periodicity: ", aeon_period_, " threshold: ", threshold,
                     " as double: ", threshold_, " cabinet size: ", cabinet_member_list.size());

      uint64_t last_block_time = current.timestamp;
      auto     current_time =
          GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM));

      if (current.block_number == 0)
      {
        last_block_time = default_start_time_;
      }

      FETCH_LOG_INFO(LOGGING_NAME, "Starting DKG with timestamp: ", last_block_time,
                     " current: ", current_time,
                     " diff: ", int64_t(current_time) - int64_t(last_block_time));

      uint64_t block_interval = 1;

      // Safe to call this multiple times
      cabinet_creator_->StartNewCabinet(cabinet_member_list, threshold,
                                        current_block_.block_number + 1,
                                        current_block_.block_number + aeon_period_,
                                        last_block_time + block_interval, current.block_entropy);
    }
  }

  beacon_->MostRecentSeen(current_block_.block_number);
  cabinet_creator_->Abort(current_block_.block_number);
}

// TODO(HUT): put block number confirmation/check here (?)
// TODO(HUT): check you're a member of qual
// TODO(HUT): turn this off when syncing(?)
NextBlockPtr Consensus::GenerateNextBlock()
{
  NextBlockPtr ret;

  // Number of block we want to generate
  uint64_t const block_number = current_block_.block_number + 1;

  ret = std::make_unique<Block>();

  // Try to get entropy for the block we are generating - is allowed to fail if we request too
  // early. Need to do entropy generation first so that we can pass the block we are generating
  // into GetBlockGenerationWeight (important for first block of each aeon which specifies the
  // qual for this aeon)
  if (EntropyGeneratorInterface::Status::OK !=
      beacon_->GenerateEntropy(block_number, ret->block_entropy))
  {
    return {};
  }

  // Note, it is important to do this here so the block when passed to ValidBlockTiming
  // is well formed
  ret->previous_hash = current_block_.hash;
  ret->block_number  = block_number;
  ret->miner         = mining_address_;
  ret->miner_id      = mining_identity_;
  ret->timestamp = GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM));
  ret->weight    = GetBlockGenerationWeight(*ret, mining_address_);

  // Note here the previous block's entropy determines miner selection
  if (!ValidBlockTiming(current_block_, *ret))
  {
    return {};
  }

  if (notarisation_)
  {
    // Add notarisation to block
    auto notarisation = notarisation_->GetAggregateNotarisation(current_block_);
    if (current_block_.block_number != 0 && notarisation.first.isZero())
    {
      // Notarisation for head of chain is not ready yet so wait
      return {};
    }
    ret->block_entropy.block_notarisation = notarisation;

    // Notify notarisation of new valid block
    notarisation_->NotariseBlock(*ret);
  }

  return ret;
}

// Helper function to determine whether the block has been signed correctly
bool BlockSignedByQualMember(fetch::ledger::Block const &block)
{
  assert(!block.hash.empty());

  // Is in qual check
  if (block.block_entropy.qualified.find(block.miner_id.identifier()) ==
      block.block_entropy.qualified.end())
  {
    return false;
  }

  return fetch::crypto::Verifier::Verify(block.miner_id, block.hash, block.miner_signature);
}

/**
 * Checks all cabinet members have submitted a notarisation key which has been signed correctly
 */
bool ValidNotarisationKeys(Consensus::BlockEntropy::Cabinet const &             cabinet,
                           Consensus::BlockEntropy::AeonNotarisationKeys const &notarisation_keys)
{
  for (auto const &member : cabinet)
  {
    if (notarisation_keys.find(member) == notarisation_keys.end() ||
        !fetch::crypto::Verifier::Verify(fetch::crypto::Identity(member),
                                         notarisation_keys.at(member).first.getStr(),
                                         notarisation_keys.at(member).second))
    {
      return false;
    }
  }
  return true;
}

/**
 * Given a block entropy, determine whether it has been signed off on
 * by enough qualified stakers.
 */
bool Consensus::EnoughQualSigned(BlockEntropy const &block_entropy) const
{
  FETCH_UNUSED(block_entropy);
  // TODO(HUT): Here, the following checks will be performed (awaits Ed's changes):
  // - Given the appropriate cabinet,
  // - Are members of qual in this cabinet
  // - Does qual meet threshold requirements: Are there enough members of qual, total
  // - Have enough qual signed the block
  return true;
}

Status Consensus::ValidBlock(Block const &current) const
{
  Status ret = Status::YES;

  // TODO(HUT): more thorough checks for genesis needed
  if (current.block_number == 0)
  {
    return Status::YES;
  }

  auto const block_preceeding = GetBlockPriorTo(current, chain_);

  // This will happen if the block is loose
  if (!block_preceeding)
  {
    return Status::NO;
  }

  if (!BlockSignedByQualMember(current))
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Saw block not signed by a member of qual! Block: ", current.block_number);
    return Status::NO;
  }

  BlockEntropy::Cabinet        qualified_cabinet;
  BlockEntropy::GroupPublicKey group_pub_key;

  // If this block would be the start of a new aeon, need to check the stakers for the prev. block
  if (ShouldTriggerAeon(block_preceeding->block_number, aeon_period_))
  {
    auto const &block_entropy = current.block_entropy;

    if (!block_entropy.IsAeonBeginning())
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Found block that didn't create a new aeon when it should have!");
      return Status::NO;
    }

    // Check that the members of qual have all signed correctly
    if (!EnoughQualSigned(block_entropy))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Received a block with a bad aeon starting point!");
    }

    // Check that the members of qual meet threshold requirements
    qualified_cabinet = block_entropy.qualified;
    group_pub_key     = block_entropy.group_public_key;

    if (notarisation_ &&
        !ValidNotarisationKeys(qualified_cabinet, block_entropy.aeon_notarisation_keys))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Found block whose notarisation keys are not valid");
      return Status::NO;
    }
  }
  else
  {
    auto beginning_of_aeon = GetBeginningOfAeon(current, chain_);
    qualified_cabinet      = beginning_of_aeon.block_entropy.qualified;
    group_pub_key          = beginning_of_aeon.block_entropy.group_public_key;
  }

  if (!dkg::BeaconManager::Verify(group_pub_key, block_preceeding->block_entropy.EntropyAsSHA256(),
                                  current.block_entropy.group_signature))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Found block whose entropy isn't a signature of the previous!");
    return Status::NO;
  }

  if (!VerifyNotarisation(current))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Found block whose notarisation is not valid");
    return Status::NO;
  }

  // Perform the time checks (also qual adherence). Note, this check should be last, as the checking
  // logic relies on a well formed block.
  if (!ValidBlockTiming(*block_preceeding, current))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Found block with bad timings!");
    return Status::NO;
  }

  return ret;
}

void Consensus::Reset(StakeSnapshot const &snapshot, StorageInterface &storage)
{
  cabinet_history_[0] = stake_->Reset(snapshot, max_cabinet_size_);

  if (cabinet_history_.find(0) == cabinet_history_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No cabinet history found for block when resetting.");
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Resetting stake aggregate...");

  // additionally since we need to flush these changes to disk
  bool const success = stake_->Save(storage);

  if (success)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Resetting stake aggregate...complete");
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Resetting stake aggregate...FAILED");
  }
}

void Consensus::SetMaxCabinetSize(uint16_t size)
{
  max_cabinet_size_ = size;
}

StakeManagerPtr Consensus::stake()
{
  return stake_;
}

void Consensus::SetAeonPeriod(uint16_t aeon_period)
{
  aeon_period_ = aeon_period;
}

void Consensus::SetBlockInterval(uint64_t block_interval_ms)
{
  block_interval_ms_ = block_interval_ms;
}

void Consensus::SetDefaultStartTime(uint64_t default_start_time)
{
  default_start_time_ = default_start_time;
}

void Consensus::AddCabinetToHistory(uint64_t block_number, CabinetPtr const &cabinet)
{
  cabinet_history_[block_number] = cabinet;
  TrimToSize(cabinet_history_, HISTORY_LENGTH);
}
