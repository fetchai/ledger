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

#include "core/random/lcg.hpp"
#include "crypto/identity.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "random_address.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace {

using fetch::ledger::StakeSnapshot;
using fetch::crypto::Identity;

using RNG              = fetch::random::LinearCongruentialGenerator;
using StakeSnapshotPtr = std::unique_ptr<StakeSnapshot>;
using StakeMap         = std::unordered_map<Identity, uint64_t>;
using IdentitySet      = std::unordered_set<Identity>;

class StakeSnapshotTests : public ::testing::Test
{
protected:
  static constexpr uint64_t MAXIMUM_SINGLE_STAKE = 10000;

  void SetUp() override
  {
    rng_.Seed(42);
    snapshot_ = std::make_unique<StakeSnapshot>();
  }

  void TearDown() override
  {
    snapshot_.reset();
  }

  StakeMap GenerateRandomStakePool(std::size_t count)
  {
    StakeMap map{};

    for (std::size_t i = 0; i < count; ++i)
    {
      auto const identity = GenerateRandomIdentity(rng_);
      uint64_t   stake    = rng_() % MAXIMUM_SINGLE_STAKE;

      // randomness is not random enough
      if (map.find(identity) != map.end())
      {
        return {};
      }

      // update our record
      map[identity] = stake;

      // update the stake tracker
      snapshot_->UpdateStake(identity, stake);
    }

    return map;
  }

  RNG              rng_;
  StakeSnapshotPtr snapshot_;
};

TEST_F(StakeSnapshotTests, CheckStakeGenerate)
{
  // generate a random stake pool
  auto const pool = GenerateRandomStakePool(200);
  ASSERT_EQ(200, pool.size());

  // ensure the stakes have been generated correctly
  uint64_t aggregate_stake{0};
  for (auto const &element : pool)
  {
    auto const retrieved_stake = snapshot_->LookupStake(element.first);
    EXPECT_EQ(element.second, retrieved_stake);
    aggregate_stake += retrieved_stake;
  }

  EXPECT_EQ(aggregate_stake, snapshot_->total_stake());

  // make a reference sample
  auto const reference = snapshot_->BuildCabinet(42, 4);
  ASSERT_TRUE(static_cast<bool>(reference));
  ASSERT_EQ(4, reference->size());

  // basic check to see if it is deterministic
  for (std::size_t i = 0; i < 5; ++i)
  {
    auto const other = snapshot_->BuildCabinet(42, 4);

    ASSERT_TRUE(static_cast<bool>(other));
    EXPECT_EQ(*reference, *other);
  }

  // check that the reference sample is unique
  IdentitySet identity_set{};
  for (auto const &identity : *reference)
  {
    identity_set.emplace(identity);
  }
  EXPECT_EQ(identity_set.size(), reference->size());
}

TEST_F(StakeSnapshotTests, CheckStateModifications)
{
  auto const identity1 = GenerateRandomIdentity(rng_);
  auto const identity2 = GenerateRandomIdentity(rng_);
  auto const identity3 = GenerateRandomIdentity(rng_);
  auto const identity4 = GenerateRandomIdentity(rng_);

  // uniform staking
  snapshot_->UpdateStake(identity1, 500);
  snapshot_->UpdateStake(identity2, 500);
  snapshot_->UpdateStake(identity3, 500);
  snapshot_->UpdateStake(identity4, 500);

  ASSERT_EQ(2000, snapshot_->total_stake());
  ASSERT_EQ(4, snapshot_->size());

  snapshot_->UpdateStake(identity1, 1000);
  ASSERT_EQ(2500, snapshot_->total_stake());
  ASSERT_EQ(4, snapshot_->size());

  snapshot_->UpdateStake(identity2, 250);
  ASSERT_EQ(2250, snapshot_->total_stake());
  ASSERT_EQ(4, snapshot_->size());

  // no change
  snapshot_->UpdateStake(identity3, 500);
  ASSERT_EQ(2250, snapshot_->total_stake());
  ASSERT_EQ(4, snapshot_->size());

  // removing stake
  snapshot_->UpdateStake(identity4, 0);
  ASSERT_EQ(1750, snapshot_->total_stake());
  ASSERT_EQ(3, snapshot_->size());
}

TEST_F(StakeSnapshotTests, TooSmallSampleSize)
{
  auto const pool   = GenerateRandomStakePool(3);
  auto const sample = snapshot_->BuildCabinet(200, 10);

  ASSERT_TRUE(static_cast<bool>(sample));
  ASSERT_EQ(pool.size(), sample->size());
}

}  // namespace
