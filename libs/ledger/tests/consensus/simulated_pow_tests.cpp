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

#include "crypto/ecdsa.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/consensus/simulated_pow_consensus.hpp"
#include "moment/deadline_timer.hpp"

#include "gtest/gtest.h"

using fetch::moment::DeadlineTimer;
using fetch::ledger::Block;
using fetch::crypto::ECDSASigner;

// Verify that the simulated POW is working by driving the main cycle, which is
// to update with the most recently seen block, and then attempt to generate
// a block (fails until block interval has passed since last block)
TEST(ledger_simulated_pow_gtest, test_block_emission)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  uint64_t const block_interval_ms = 2000;

  // generate a public/private key pair for the constructor (not needed for the test)
  auto signer = std::make_shared<ECDSASigner>();

  auto consensus =
      std::make_shared<fetch::ledger::SimulatedPowConsensus>(signer->identity(), block_interval_ms);

  std::shared_ptr<Block> block = std::make_shared<Block>();
  block->timestamp = GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM));
  std::vector<Block> all_blocks;

  DeadlineTimer timer_to_proceed_{"pow:test"};
  timer_to_proceed_.Restart(std::chrono::milliseconds{6000});

  // Generate around 10 blocks given our time interval
  while (!timer_to_proceed_.HasExpired())
  {
    if (block)
    {
      consensus->UpdateCurrentBlock(*block);
      all_blocks.push_back(*block);
      block.reset();
    }

    // Failure will set this to a nullptr
    block = consensus->GenerateNextBlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  ASSERT_TRUE(all_blocks.size() >= 2 && all_blocks.size() <= 4);
}

TEST(ledger_simulated_pow_gtest, test_disable_functionality)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  uint64_t const block_interval_ms = 0;

  // generate a public/private key pair for the constructor (not needed for the test)
  auto signer = std::make_shared<ECDSASigner>();

  auto consensus =
      std::make_shared<fetch::ledger::SimulatedPowConsensus>(signer->identity(), block_interval_ms);

  std::shared_ptr<Block> block = std::make_shared<Block>();
  block->timestamp = GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM));
  std::vector<Block> all_blocks;

  DeadlineTimer timer_to_proceed_{"pow:test"};
  timer_to_proceed_.Restart(std::chrono::milliseconds{1000});

  // Generate around 10 blocks given our time interval
  while (!timer_to_proceed_.HasExpired())
  {
    if (block)
    {
      consensus->UpdateCurrentBlock(*block);
      all_blocks.push_back(*block);
      block.reset();
    }

    // Failure will set this to a nullptr
    block = consensus->GenerateNextBlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  // Should only contain 'genesis'
  ASSERT_TRUE(all_blocks.size() == 1);
}
