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

// namespace {

constexpr char const *LOGGING_NAME = "SimulatedPowConsensus";

using SimulatedPowConsensus = fetch::ledger::SimulatedPowConsensus;
using NextBlockPtr          = SimulatedPowConsensus::NextBlockPtr;
using Status                = SimulatedPowConsensus::Status;

using fetch::ledger::Block;

uint64_t GetPoissonSample(uint64_t range, double mean_of_distribution)
{
  thread_local std::random_device rd;
  thread_local std::mt19937       rng(rd());

  using Distribution = std::poisson_distribution<uint64_t>;

  Distribution dist(mean_of_distribution);

  auto ret = std::min(dist(rng), range);;

  FETCH_LOG_INFO(LOGGING_NAME, "Ret: ", ret);

  return ret;
}

SimulatedPowConsensus::SimulatedPowConsensus(Identity mining_identity, uint64_t block_interval_ms)
  : mining_identity_{std::move(mining_identity)}
  , block_interval_ms_{block_interval_ms}
{}

void SimulatedPowConsensus::UpdateCurrentBlock(Block const &current)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Updating new current block: ", current.hash.ToHex(), " num: ", current.block_number);

  if(current == current_block_)
  {
    return;
  }

  current_block_ = current;

  if (current.miner_id.identifier().empty() && current.block_number != 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Received block with empty miner ID for simulated POW: ");
  }

  double mean_time_to_block = block_interval_ms_;

  if(current.miner_id == mining_identity_)
  {
    mean_time_to_block *= 1.1;
  }

  uint64_t time_to_wait_ms =
      static_cast<uint64_t>(GetPoissonSample(30000, mean_time_to_block));

  FETCH_LOG_INFO(LOGGING_NAME, "Generating time to wait (ms): ", time_to_wait_ms,
                 " mean: ", mean_time_to_block, " block time: ", block_interval_ms_);

  // Generate a block probalistically based on the previous block time and the number
  // of other peers seen so far (bounded to 30s)
  decided_next_timestamp_ms_ = (current.timestamp * 1000) + time_to_wait_ms;
}

NextBlockPtr SimulatedPowConsensus::GenerateNextBlock()
{
  NextBlockPtr ret;

  uint64_t current_time_ms = GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM), moment::TimeAccuracy::MILLISECONDS);

  if (!(current_time_ms > decided_next_timestamp_ms_))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Waiting before producing block. Milliseconds to wait: ",
                   decided_next_timestamp_ms_ - current_time_ms);
    return ret;
  }

  // Number of block we want to generate
  uint64_t const block_number = current_block_.block_number + 1;

  ret = std::make_unique<Block>();

  // Note, it is important to do this here so the block when passed to ValidBlockTiming
  // is well formed
  ret->previous_hash = current_block_.hash;
  ret->block_number  = block_number;
  /* ret->miner         = mining_address_; */
  ret->miner_id  = mining_identity_;
  ret->timestamp = GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM));
  ret->weight    = GetPoissonSample(200, 50);

  return ret;
}

Status SimulatedPowConsensus::ValidBlock(Block const & /*current*/) const
{
  return Status::YES;
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

void SimulatedPowConsensus::SetDefaultStartTime(uint64_t /*default_start_time*/)
{}
