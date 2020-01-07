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

#include "ledger/consensus/simulated_pow_consensus.hpp"

#include <ctime>
#include <random>
#include <utility>

/**
 * This simulated pow consensus is used for testing, when it is not needed to
 * do the full scheme, as key generation takes a long time.
 *
 * In order to simulate pow, the consensus will probalistically generate blocks
 * so as to emulate pow. Blocks are all assumed to be valid.
 * Forks are much more likely in this scheme and there are no guarantees
 * around block time.
 *
 */

constexpr char const *LOGGING_NAME = "SimulatedPowConsensus";

using SimulatedPowConsensus = fetch::ledger::SimulatedPowConsensus;
using NextBlockPtr          = SimulatedPowConsensus::NextBlockPtr;
using Status                = SimulatedPowConsensus::Status;

SimulatedPowConsensus::SimulatedPowConsensus(Identity mining_identity, uint64_t block_interval_ms,
                                             MainChain const &chain)
  : mining_identity_{std::move(mining_identity)}
  , block_interval_ms_{block_interval_ms}
  , chain_{chain}
{}

uint64_t GetPoissonSample(uint64_t range, double mean_of_distribution)
{
  thread_local std::random_device rd;
  thread_local std::mt19937       rng(rd());

  using Distribution = std::poisson_distribution<uint64_t>;

  Distribution dist(mean_of_distribution);

  return std::min(dist(rng), range);
}

void SimulatedPowConsensus::UpdateCurrentBlock(Block const &current)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Updating new current block: ", current.hash.ToHex(),
                  " num: ", current.block_number);

  if (current == current_block_)
  {
    return;
  }

  current_block_ = current;

  if (current.miner_id.identifier().empty() && current.block_number != 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Received block with empty miner ID for simulated POW: ");
  }

  auto mean_time_to_block = static_cast<double>(block_interval_ms_);

  // So as to provoke the possibility of forks, make it slightly less likely the miner
  // generates two blocks in a row
  if (current.miner_id == mining_identity_)
  {
    mean_time_to_block *= 1.05;
  }

  auto time_to_wait_ms = static_cast<uint64_t>(GetPoissonSample(30000, mean_time_to_block));

  bool disabled =
      block_interval_ms_ == std::numeric_limits<uint64_t>::max() || block_interval_ms_ == 0;

  if (disabled)
  {
    decided_next_timestamp_ms_ = std::numeric_limits<uint64_t>::max();
  }
  else
  {
    // Generate a block probalistically based on the previous block time and the number
    // of other peers seen so far (bounded to 30s)
    decided_next_timestamp_ms_ = (current.timestamp * 1000) + time_to_wait_ms;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Generating time to wait (ms): ", time_to_wait_ms,
                 " mean: ", mean_time_to_block, " block time: ", block_interval_ms_,
                 " decided: ", decided_next_timestamp_ms_);
}

NextBlockPtr SimulatedPowConsensus::GenerateNextBlock()
{
  NextBlockPtr ret;

  uint64_t current_time_ms =
      GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM),
              moment::TimeAccuracy::MILLISECONDS);

  if (!(current_time_ms > decided_next_timestamp_ms_) && !forcibly_generate_next_)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Waiting before producing block. Milliseconds to wait: ",
                    decided_next_timestamp_ms_ - current_time_ms);
    return ret;
  }

  forcibly_generate_next_ = false;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Generating block. Current time: ", current_time_ms,
                  " deadline: ", decided_next_timestamp_ms_);

  // Round up to the nearest second for block timestamp
  auto const current_time_s_ceiled =
      static_cast<uint64_t>(std::ceil(static_cast<double>(current_time_ms) / 1000.0));

  // Number of block we want to generate
  uint64_t const block_number = current_block_.block_number + 1;

  ret = std::make_unique<Block>();

  // Note, it is important to do this here so the block when passed to ValidBlockTiming
  // is well formed
  ret->previous_hash = current_block_.hash;
  ret->block_number  = block_number;
  ret->miner_id      = mining_identity_;
  ret->timestamp     = current_time_s_ceiled;
  ret->weight        = GetPoissonSample(200, 50);

  return ret;
}

Status SimulatedPowConsensus::ValidBlock(Block const &current) const
{
  // Loose blocks can not be valid, and the chain must be there.
  if (!chain_.GetBlock(current.previous_hash))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Attempted to validate loose block. Discarding.");
    return Status::NO;
  }

  return Status::YES;
}

void SimulatedPowConsensus::TriggerBlockGeneration()
{
  forcibly_generate_next_ = true;
}

void SimulatedPowConsensus::SetBlockInterval(uint64_t block_interval_ms)
{
  block_interval_ms_ = block_interval_ms;
}

void SimulatedPowConsensus::SetMaxCabinetSize(uint16_t /*max_cabinet_size*/)
{}

void SimulatedPowConsensus::SetAeonPeriod(uint16_t /*aeon_period*/)
{}

void SimulatedPowConsensus::Reset(StakeSnapshot const & /*snapshot*/,
                                  StorageInterface & /*storage*/)
{}

void SimulatedPowConsensus::Reset(StakeSnapshot const & /*snapshot*/)
{}

void SimulatedPowConsensus::SetDefaultStartTime(uint64_t /*default_start_time*/)
{}

void SimulatedPowConsensus::SetWhitelist(Minerwhitelist const & /*whitelist*/)
{}
