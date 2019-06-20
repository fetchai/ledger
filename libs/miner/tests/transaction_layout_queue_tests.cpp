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

#include "tx_generator.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/random/lcg.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/chain/transaction_layout.hpp"
#include "miner/transaction_layout_queue.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::random::LinearCongruentialGenerator;
using fetch::miner::TransactionLayoutQueue;
using TransactionLayoutQueuePtr = std::unique_ptr<TransactionLayoutQueue>;
using fetch::ledger::Digest;

class TransactionLayoutQueueTests : public ::testing::Test
{
protected:
  using Rng     = LinearCongruentialGenerator;
  using RngWord = Rng::random_type;

  void SetUp() override
  {
    queue_ = std::make_unique<TransactionLayoutQueue>();
    generator_.Seed();
  }

  void TearDown() override
  {
    queue_.reset();
  }

  TransactionLayoutQueuePtr queue_;
  TransactionGenerator      generator_{};
};

template <typename Container, typename Value>
bool IsIn(Container const &container, Value const &value)
{
  return std::find(container.begin(), container.end(), value) != container.end();
}

template <typename Iterator>
Iterator Advance(Iterator current, std::size_t amount)
{
  std::advance(current, static_cast<typename Iterator::difference_type>(amount));
  return current;
}

TEST_F(TransactionLayoutQueueTests, CheckBasicAdditions)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  // add the transaction and check expectations
  EXPECT_EQ(queue_->size(), 0);
  EXPECT_FALSE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));

  EXPECT_TRUE(queue_->Add(tx1));

  EXPECT_EQ(queue_->size(), 1u);
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));

  EXPECT_TRUE(queue_->Add(tx2));

  EXPECT_EQ(queue_->size(), 2u);
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));

  EXPECT_TRUE(queue_->Add(tx3));

  EXPECT_EQ(queue_->size(), 3u);
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_TRUE(IsIn(*queue_, tx3));
}

TEST_F(TransactionLayoutQueueTests, CheckDuplicateRejection)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_FALSE(queue_->Add(tx1));
  EXPECT_FALSE(queue_->Add(tx1));
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));

  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_FALSE(queue_->Add(tx2));
  EXPECT_FALSE(queue_->Add(tx2));
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));

  EXPECT_TRUE(queue_->Add(tx3));
  EXPECT_FALSE(queue_->Add(tx3));
  EXPECT_FALSE(queue_->Add(tx3));
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_TRUE(IsIn(*queue_, tx3));
}

TEST_F(TransactionLayoutQueueTests, CheckSingleRemovals)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_TRUE(queue_->Add(tx3));

  // check removals
  EXPECT_TRUE(queue_->Remove(tx1.digest()));
  EXPECT_EQ(queue_->size(), 2u);
  EXPECT_FALSE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_TRUE(IsIn(*queue_, tx3));

  EXPECT_TRUE(queue_->Remove(tx2.digest()));
  EXPECT_EQ(queue_->size(), 1u);
  EXPECT_FALSE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_TRUE(IsIn(*queue_, tx3));

  EXPECT_TRUE(queue_->Remove(tx3.digest()));
  EXPECT_EQ(queue_->size(), 0);
  EXPECT_FALSE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));
}

TEST_F(TransactionLayoutQueueTests, CheckSetRemoval1)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_TRUE(queue_->Add(tx3));

  // check removals
  EXPECT_TRUE(queue_->Remove({tx1.digest(), tx2.digest()}));
  EXPECT_EQ(queue_->size(), 1u);
  EXPECT_FALSE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_TRUE(IsIn(*queue_, tx3));
}

TEST_F(TransactionLayoutQueueTests, CheckSetRemoval2)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_TRUE(queue_->Add(tx3));

  // check removals
  EXPECT_TRUE(queue_->Remove({tx1.digest(), tx3.digest()}));
  EXPECT_EQ(queue_->size(), 1u);
  EXPECT_FALSE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));
}

TEST_F(TransactionLayoutQueueTests, CheckSetRemoval3)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_TRUE(queue_->Add(tx3));

  // check removals
  EXPECT_TRUE(queue_->Remove({tx2.digest(), tx3.digest()}));
  EXPECT_EQ(queue_->size(), 1u);
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));
}

TEST_F(TransactionLayoutQueueTests, CheckSetRemovalAll)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_TRUE(queue_->Add(tx3));

  // check removals
  EXPECT_TRUE(queue_->Remove({tx1.digest(), tx2.digest(), tx3.digest()}));
  EXPECT_EQ(queue_->size(), 0);
  EXPECT_FALSE(IsIn(*queue_, tx1));
  EXPECT_FALSE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));
}

TEST_F(TransactionLayoutQueueTests, CheckSplice)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_TRUE(queue_->Add(tx3));
  EXPECT_EQ(queue_->size(), 3u);

  TransactionLayoutQueue other;

  auto const tx4 = generator_(2);
  auto const tx5 = generator_(2);
  auto const tx6 = generator_(2);

  EXPECT_TRUE(other.Add(tx4));
  EXPECT_TRUE(other.Add(tx5));
  EXPECT_TRUE(other.Add(tx6));
  EXPECT_EQ(other.size(), 3u);

  queue_->Splice(other);

  EXPECT_EQ(other.size(), 0);

  EXPECT_EQ(queue_->size(), 6u);
  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_TRUE(IsIn(*queue_, tx3));
  EXPECT_TRUE(IsIn(*queue_, tx4));
  EXPECT_TRUE(IsIn(*queue_, tx5));
  EXPECT_TRUE(IsIn(*queue_, tx6));
}

TEST_F(TransactionLayoutQueueTests, CheckSorting)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);

  EXPECT_TRUE(queue_->Add(tx1));
  EXPECT_TRUE(queue_->Add(tx2));
  EXPECT_TRUE(queue_->Add(tx3));

  // sort the queue by charge
  queue_->Sort([](auto const &a, auto const &b) { return a.charge() > b.charge(); });

  ASSERT_EQ(queue_->size(), 3u);

  auto it = queue_->cbegin();
  EXPECT_EQ((it++)->digest(), tx3.digest());
  EXPECT_EQ((it++)->digest(), tx2.digest());
  EXPECT_EQ((it++)->digest(), tx1.digest());
}

TEST_F(TransactionLayoutQueueTests, CheckSubSplicing)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);
  auto const tx4 = generator_(2);

  // create a basic queue
  TransactionLayoutQueue other;
  EXPECT_TRUE(other.Add(tx1));
  EXPECT_TRUE(other.Add(tx2));
  EXPECT_TRUE(other.Add(tx3));
  EXPECT_TRUE(other.Add(tx4));

  // splice part of the queue
  auto const start = other.begin();
  auto const end   = Advance(start, 2);
  queue_->Splice(other, start, end);

  EXPECT_EQ(other.size(), 2u);
  EXPECT_EQ(queue_->size(), 2u);

  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));
  EXPECT_FALSE(IsIn(*queue_, tx4));

  EXPECT_FALSE(IsIn(other, tx1));
  EXPECT_FALSE(IsIn(other, tx2));
  EXPECT_TRUE(IsIn(other, tx3));
  EXPECT_TRUE(IsIn(other, tx4));
}

TEST_F(TransactionLayoutQueueTests, CheckDuplicateSubSplicing)
{
  // generate a series of transactions
  auto const tx1 = generator_(2);
  auto const tx2 = generator_(2);
  auto const tx3 = generator_(2);
  auto const tx4 = generator_(2);

  // create a basic queue
  TransactionLayoutQueue other;
  EXPECT_TRUE(other.Add(tx1));
  EXPECT_TRUE(other.Add(tx2));
  EXPECT_TRUE(other.Add(tx3));
  EXPECT_TRUE(other.Add(tx4));

  // add the first transaction to the queue to make the splice harder
  EXPECT_TRUE(queue_->Add(tx1));

  // splice part of the queue
  auto const start = other.begin();
  auto const end   = Advance(start, 2);
  queue_->Splice(other, start, end);

  EXPECT_EQ(other.size(), 2u);
  EXPECT_EQ(queue_->size(), 2u);

  EXPECT_TRUE(IsIn(*queue_, tx1));
  EXPECT_TRUE(IsIn(*queue_, tx2));
  EXPECT_FALSE(IsIn(*queue_, tx3));
  EXPECT_FALSE(IsIn(*queue_, tx4));

  EXPECT_FALSE(IsIn(other, tx1));
  EXPECT_FALSE(IsIn(other, tx2));
  EXPECT_TRUE(IsIn(other, tx3));
  EXPECT_TRUE(IsIn(other, tx4));
}

}  // namespace
