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
#include "ledger/consensus/stake_tracker.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace {

using fetch::ledger::StakeTracker;
using fetch::ledger::Address;

using RNG             = fetch::random::LinearCongruentialGenerator;
using StakeTrackerPtr = std::unique_ptr<StakeTracker>;
using StakeMap        = std::unordered_map<Address, uint64_t>;
using AddressSet      = std::unordered_set<Address>;

class StakeTrackerTests : public ::testing::Test
{
protected:
  static constexpr uint64_t MAXIMUM_SINGLE_STAKE = 10000;

  void SetUp() override
  {
    rng_.Seed(42);
    stake_tracker_ = std::make_unique<StakeTracker>();
  }

  void TearDown() override
  {
    stake_tracker_.reset();
  }

  Address GenerateRandomAddress()
  {
    static constexpr std::size_t ADDRESS_WORD_SIZE = Address::RAW_LENGTH / sizeof(RNG::random_type);
    static_assert((Address::RAW_LENGTH % sizeof(RNG::random_type)) == 0, "");

    Address::RawAddress raw_address;
    auto *raw = reinterpret_cast<RNG::random_type*>(raw_address.data());

    for (std::size_t i = 0; i < ADDRESS_WORD_SIZE; ++i)
    {
      raw[i] = rng_();
    }

    return Address{raw_address};
  }

  StakeMap GenerateRandomStakePool(std::size_t count)
  {
    StakeMap map{};

    for (std::size_t i = 0; i < count; ++i)
    {
      auto const address = GenerateRandomAddress();
      uint64_t   stake   = rng_() % MAXIMUM_SINGLE_STAKE;

      // randomness is not random enough
      if (map.find(address) != map.end())
      {
        return {};
      }

      // update our record
      map[address] = stake;

      // update the stake tracker
      stake_tracker_->UpdateStake(address, stake);
    }

    return map;
  }

  RNG rng_;
  StakeTrackerPtr stake_tracker_;
};

TEST_F(StakeTrackerTests, CheckStakeGenerate)
{
  // generate a random stake pool
  auto const pool = GenerateRandomStakePool(200);
  ASSERT_EQ(200, pool.size());

  // ensure the stakes have been generated correctly
  uint64_t aggregate_stake{0};
  for (auto const &element : pool)
  {
    auto const retrieved_stake = stake_tracker_->LookupStake(element.first);
    EXPECT_EQ(element.second, retrieved_stake);
    aggregate_stake += retrieved_stake;
  }

  EXPECT_EQ(aggregate_stake, stake_tracker_->total_stake());

  // make a reference sample
  auto const reference = stake_tracker_->Sample(42, 4);
  ASSERT_EQ(4, reference.size());

  // basic check to see if it is deterministic
  for (std::size_t i = 0; i < 5; ++i)
  {
    auto const other = stake_tracker_->Sample(42, 4);

    EXPECT_EQ(reference, other);
  }

  // check that the reference sample is unique
  AddressSet address_set{};
  for (auto const &address : reference)
  {
    address_set.emplace(address);
  }
  EXPECT_EQ(address_set.size(), reference.size());
}

TEST_F(StakeTrackerTests, CheckStateModifications)
{
  auto const address1 = GenerateRandomAddress();
  auto const address2 = GenerateRandomAddress();
  auto const address3 = GenerateRandomAddress();
  auto const address4 = GenerateRandomAddress();

  // uniform staking
  stake_tracker_->UpdateStake(address1, 500);
  stake_tracker_->UpdateStake(address2, 500);
  stake_tracker_->UpdateStake(address3, 500);
  stake_tracker_->UpdateStake(address4, 500);

  ASSERT_EQ(2000, stake_tracker_->total_stake());
  ASSERT_EQ(4, stake_tracker_->size());

  stake_tracker_->UpdateStake(address1, 1000);
  ASSERT_EQ(2500, stake_tracker_->total_stake());
  ASSERT_EQ(4, stake_tracker_->size());

  stake_tracker_->UpdateStake(address2, 250);
  ASSERT_EQ(2250, stake_tracker_->total_stake());
  ASSERT_EQ(4, stake_tracker_->size());

  // no change
  stake_tracker_->UpdateStake(address3, 500);
  ASSERT_EQ(2250, stake_tracker_->total_stake());
  ASSERT_EQ(4, stake_tracker_->size());

  // removing stake
  stake_tracker_->UpdateStake(address4, 0);
  ASSERT_EQ(1750, stake_tracker_->total_stake());
  ASSERT_EQ(3, stake_tracker_->size());
}

TEST_F(StakeTrackerTests, TooSmallSampleSize)
{
  auto const pool = GenerateRandomStakePool(3);
  auto const sample = stake_tracker_->Sample(200, 10);

  ASSERT_EQ(pool.size(), sample.size());
}

} // namespace