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
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/consensus/stake_update_queue.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::ledger::StakeSnapshot;
using fetch::ledger::StakeUpdateQueue;

using RNG                 = fetch::random::LinearCongruentialGenerator;
using StakeUpdateQueuePtr = std::unique_ptr<StakeUpdateQueue>;
using StakeSnapshotPtr    = std::shared_ptr<StakeSnapshot>;

template <typename Container, typename Value>
bool IsIn(Container const &container, Value const &value)
{
  return container.find(value) != container.end();
}

class StakeUpdateQueueTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rng_.Seed(42);
    stake_update_queue_ = std::make_unique<StakeUpdateQueue>();
  }

  void TearDown() override
  {
    stake_update_queue_.reset();
  }

  StakeUpdateQueuePtr stake_update_queue_;
  RNG                 rng_;
};

TEST_F(StakeUpdateQueueTests, SimpleCheck)
{
  auto const address1 = GenerateRandomAddress(rng_);
  auto const address2 = GenerateRandomAddress(rng_);
  auto const address3 = GenerateRandomAddress(rng_);

  stake_update_queue_->AddStakeUpdate(10, address1, 500);
  stake_update_queue_->AddStakeUpdate(11, address2, 500);
  stake_update_queue_->AddStakeUpdate(12, address3, 500);
  EXPECT_EQ(3, stake_update_queue_->size());

  // check to make sure the update map has been set correctly
  stake_update_queue_->VisitUnderlyingQueue([&](auto const &map) {
    EXPECT_EQ(3, map.size());

    EXPECT_TRUE(IsIn(map, 10u));
    EXPECT_TRUE(IsIn(map, 11u));
    EXPECT_TRUE(IsIn(map, 12u));

    EXPECT_TRUE(IsIn(map.at(10), address1));
    EXPECT_EQ(1, map.at(10).size());
    EXPECT_TRUE(IsIn(map.at(11), address2));
    EXPECT_EQ(1, map.at(11).size());
    EXPECT_TRUE(IsIn(map.at(12), address3));
    EXPECT_EQ(1, map.at(12).size());

    EXPECT_EQ(500, map.at(10).at(address1));
    EXPECT_EQ(500, map.at(11).at(address2));
    EXPECT_EQ(500, map.at(12).at(address3));
  });

  StakeSnapshotPtr current_snapshot = std::make_shared<StakeSnapshot>();
  ASSERT_EQ(0, current_snapshot->size());
  ASSERT_EQ(0, current_snapshot->total_stake());
  EXPECT_EQ(3, stake_update_queue_->size());

  // apply some state updates
  StakeSnapshotPtr next_snapshot{};
  ASSERT_FALSE(stake_update_queue_->ApplyUpdates(9, current_snapshot, next_snapshot));

  ASSERT_EQ(0, current_snapshot->size());
  ASSERT_EQ(0, current_snapshot->total_stake());
  ASSERT_FALSE(static_cast<bool>(next_snapshot));
  EXPECT_EQ(3, stake_update_queue_->size());

  // apply some state updates
  ASSERT_TRUE(stake_update_queue_->ApplyUpdates(10, current_snapshot, next_snapshot));

  // check the current snapshot has not been changed
  ASSERT_EQ(0, current_snapshot->size());
  ASSERT_EQ(0, current_snapshot->total_stake());

  // check that the next snapshot has the new changes
  ASSERT_TRUE(static_cast<bool>(next_snapshot));
  ASSERT_EQ(1, next_snapshot->size());
  ASSERT_EQ(500, next_snapshot->total_stake());

  EXPECT_EQ(2, stake_update_queue_->size());
  std::swap(current_snapshot, next_snapshot);

  // apply some state updates
  ASSERT_TRUE(stake_update_queue_->ApplyUpdates(12, current_snapshot, next_snapshot));

  ASSERT_EQ(1, current_snapshot->size());
  ASSERT_EQ(500, current_snapshot->total_stake());

  ASSERT_EQ(2, next_snapshot->size());
  ASSERT_EQ(1000, next_snapshot->total_stake());

  EXPECT_EQ(0, stake_update_queue_->size());
}

}  // namespace
