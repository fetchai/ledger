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

#include "random_address.hpp"

#include "core/random/lcg.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/consensus/naive_entropy_generator.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/stake_snapshot.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::ledger::Block;
using fetch::ledger::Address;
using fetch::ledger::StakeSnapshot;
using fetch::ledger::StakeManager;
using fetch::ledger::NaiveEntropyGenerator;

using RNG             = fetch::random::LinearCongruentialGenerator;
using StakeManagerPtr = std::unique_ptr<StakeManager>;
using EntropyPtr      = std::unique_ptr<NaiveEntropyGenerator>;
using RoundStats      = std::unordered_map<Address, std::size_t>;

constexpr char const *LOGGING_NAME = "StakeMgrTests";

bool IndexOf(std::vector<Address> const &addresses, Address const &target, std::size_t &index)
{
  bool found{false};

  std::size_t idx{0};
  for (auto const &address : addresses)
  {
    if (address == target)
    {
      index = idx;
      found = true;
      break;
    }

    ++idx;
  }

  return found;
}

std::size_t WeightOf(std::vector<Address> const &addresses, Address const &target)
{
  std::size_t weight{0};
  std::size_t index{0};
  if (IndexOf(addresses, target, index))
  {
    weight = addresses.size() - index;
  }

  return weight;
}

class StakeManagerTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rng_.Seed(2048);
    entropy_       = std::make_unique<NaiveEntropyGenerator>();
    stake_manager_ = std::make_unique<StakeManager>(*entropy_);
  }

  void TearDown() override
  {
    stake_manager_.reset();
  }

  void SimulateRounds(std::vector<Address> const &addresses, Block &block, std::size_t num_rounds,
                      std::size_t committee_size, RoundStats &stats)
  {
    ASSERT_GT(committee_size, 0);

    // initialise the stats (create entries if they don't exist)
    for (auto const &address : addresses)
    {
      stats.emplace(address, 0);
    }

    for (std::size_t round = 0; round < num_rounds; ++round)
    {
      // validate the committee vs the generation weight
      auto const committee = stake_manager_->GetCommittee(block);
      ASSERT_TRUE(static_cast<bool>(committee));
      ASSERT_EQ(committee->size(), committee_size);

      // update the statistics
      stats.at(committee->at(0)) += 1;

      for (auto const &address : addresses)
      {
        EXPECT_EQ(WeightOf(*committee, address),
                  stake_manager_->GetBlockGenerationWeight(block, address));
      }

      // "forge" the next block
      block.body.previous_hash = block.body.hash;
      block.body.hash          = GenerateRandomAddress(rng_).address();
      block.body.block_number += 1;

      stake_manager_->UpdateCurrentBlock(block);
    }
  }

  RNG             rng_;
  EntropyPtr      entropy_;
  StakeManagerPtr stake_manager_;
};

TEST_F(StakeManagerTests, CheckBasicStakeChangeScenarios)
{
  static constexpr std::size_t COMMITTEE_SIZE = 1;

  std::vector<Address> addresses = {
      GenerateRandomAddress(rng_),
      GenerateRandomAddress(rng_),
      GenerateRandomAddress(rng_),
  };

  // create the initial stake setup
  StakeSnapshot initial{};
  for (auto const &address : addresses)
  {
    initial.UpdateStake(address, 500);
  }

  // configure the stake manager
  stake_manager_->Reset(initial, COMMITTEE_SIZE);

  // create the starting blocks
  Block block;
  block.body.hash         = GenerateRandomAddress(rng_).address();
  block.body.block_number = 1;

  // simulate a number of rounds
  RoundStats stats{};
  SimulateRounds(addresses, block, 100, COMMITTEE_SIZE, stats);

  for (auto const &address : addresses)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Address: ", address.display(), " rounds: ", stats.at(address));

    EXPECT_GT(stats.at(address), 0);
  }
  FETCH_LOG_INFO(LOGGING_NAME, "---");

  // along comes another staker
  addresses.emplace_back(GenerateRandomAddress(rng_));
  stake_manager_->update_queue().AddStakeUpdate(150, addresses.back(), 500);

  stats.clear();
  SimulateRounds(addresses, block, 100, COMMITTEE_SIZE, stats);

  for (auto const &address : addresses)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Address: ", address.display(), " rounds: ", stats.at(address));

    EXPECT_GT(stats.at(address), 0);
  }
  FETCH_LOG_INFO(LOGGING_NAME, "---");

  // stakers have been removed
  for (std::size_t i = 1; i < addresses.size(); ++i)
  {
    stake_manager_->update_queue().AddStakeUpdate(250, addresses.at(i), 0);
  }

  stats.clear();
  SimulateRounds(addresses, block, 100, COMMITTEE_SIZE, stats);

  for (auto const &address : addresses)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Address: ", address.display(), " rounds: ", stats.at(address));

    EXPECT_GT(stats.at(address), 0);
  }
  FETCH_LOG_INFO(LOGGING_NAME, "---");

  stats.clear();
  SimulateRounds(addresses, block, 100, COMMITTEE_SIZE, stats);

  std::size_t idx{0};
  for (auto const &address : addresses)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Address: ", address.display(), " rounds: ", stats.at(address));

    if (idx == 0)
    {
      EXPECT_GT(stats.at(address), 0);
    }
    else
    {
      EXPECT_EQ(stats.at(address), 0);
    }

    ++idx;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "---");
}

TEST_F(StakeManagerTests, CheckCommitteeCaching)
{

}

}  // namespace