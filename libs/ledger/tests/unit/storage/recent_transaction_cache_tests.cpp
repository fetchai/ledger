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

#include "core/containers/is_in.hpp"
#include "core/digest.hpp"
#include "ledger/storage_unit/recent_transaction_cache.hpp"
#include "transaction_generator.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::DigestSet;
using fetch::core::IsIn;
using fetch::ledger::RecentTransactionsCache;

constexpr std::size_t MAX_CACHE_SIZE = 5;
constexpr uint32_t    LOG2_NUM_LANES = 1;

class RecentTransactionsCacheTests : public ::testing::Test
{
protected:
  RecentTransactionsCache cache_{MAX_CACHE_SIZE, LOG2_NUM_LANES};
  TransactionGenerator    tx_gen_;
};

TEST_F(RecentTransactionsCacheTests, CheckFillingOfCache)
{
  constexpr std::size_t NUM_TX = 2 * MAX_CACHE_SIZE;

  auto const txs = tx_gen_.GenerateRandomTxs(NUM_TX);

  for (std::size_t i = 0; i < NUM_TX; ++i)
  {
    EXPECT_EQ(cache_.GetSize(), std::min(MAX_CACHE_SIZE, i));

    // add the transaction to the cache
    cache_.Add(*txs[i]);

    EXPECT_EQ(cache_.GetSize(), std::min(MAX_CACHE_SIZE, i + 1));
  }
}

TEST_F(RecentTransactionsCacheTests, CheckFillingOfCacheOrder)
{
  constexpr std::size_t NUM_TX = 2 * MAX_CACHE_SIZE;

  // generate and add all the transactions into the cache
  auto const txs = tx_gen_.GenerateRandomTxs(NUM_TX);
  for (auto const &tx : txs)
  {
    cache_.Add(*tx);
  }

  // extract the elements from the cache
  auto const entries = cache_.Flush(NUM_TX);
  ASSERT_EQ(cache_.GetSize(), 0);
  ASSERT_EQ(entries.size(), MAX_CACHE_SIZE);

  // check the ordering of the extracted entries
  for (std::size_t i = 0; i < MAX_CACHE_SIZE; ++i)
  {
    ASSERT_EQ(entries.at(i).digest(), txs.at(NUM_TX - (i + 1))->digest());
  }
}

TEST_F(RecentTransactionsCacheTests, CheckTransactionLayouts)
{
  auto txs = tx_gen_.GenerateRandomTxs(MAX_CACHE_SIZE);

  DigestSet digests{};
  for (auto const &tx : txs)
  {
    cache_.Add(*tx);

    digests.emplace(tx->digest());
  }

  EXPECT_EQ(digests.size(), MAX_CACHE_SIZE);

  EXPECT_EQ(cache_.GetSize(), MAX_CACHE_SIZE);
  auto const cache = cache_.Flush(MAX_CACHE_SIZE);
  EXPECT_EQ(cache_.GetSize(), 0);

  // build complete list of input digests
  for (auto const &layout : cache)
  {
    EXPECT_TRUE(IsIn(digests, layout.digest()));
    EXPECT_EQ(layout.mask().size(), 1u << LOG2_NUM_LANES);
  }
}

}  // namespace
