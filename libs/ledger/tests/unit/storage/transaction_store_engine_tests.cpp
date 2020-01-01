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

#include "ledger/storage_unit/transaction_storage_engine.hpp"
#include "transaction_generator.hpp"

#include "gtest/gtest.h"

namespace {

constexpr uint32_t LANE_ID        = 0;
constexpr uint32_t LOG2_NUM_LANES = 1;

using fetch::chain::Transaction;
using fetch::ledger::TransactionStorageEngine;

class TransactionStorageEngineTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    storage_.New("tx.storage.engine.tests.db", "tx.storage.engine.tests.index.db", true);
  }

  TransactionStorageEngine storage_{LOG2_NUM_LANES, LANE_ID};
  TransactionGenerator     tx_gen_;
};

TEST_F(TransactionStorageEngineTests, CheckRecentTxIsPresent)
{
  auto const tx = tx_gen_();
  storage_.Add(*tx, true);

  EXPECT_TRUE(storage_.Has(tx->digest()));
}

TEST_F(TransactionStorageEngineTests, CheckRecentTxCanBeRetrieved)
{
  auto const tx = tx_gen_();
  storage_.Add(*tx, true);

  Transaction out{};
  EXPECT_TRUE(storage_.Get(tx->digest(), out));
  EXPECT_EQ(out.digest(), tx->digest());
}

TEST_F(TransactionStorageEngineTests, CheckRecentTxInRecentCache)
{
  auto const tx = tx_gen_();
  storage_.Add(*tx, true);

  auto const recent = storage_.GetRecent(100);
  ASSERT_EQ(recent.size(), 1);

  EXPECT_EQ(recent.at(0).digest(), tx->digest());
}

TEST_F(TransactionStorageEngineTests, CheckTxIsPresent)
{
  auto const tx = tx_gen_();
  storage_.Add(*tx, false);

  EXPECT_TRUE(storage_.Has(tx->digest()));
}

TEST_F(TransactionStorageEngineTests, CheckTxCanBeRetrieved)
{
  auto const tx = tx_gen_();
  storage_.Add(*tx, false);

  Transaction out{};
  EXPECT_TRUE(storage_.Get(tx->digest(), out));
  EXPECT_EQ(out.digest(), tx->digest());
}

TEST_F(TransactionStorageEngineTests, CheckTxNotInRecentCache)
{
  auto const tx = tx_gen_();
  storage_.Add(*tx, false);

  auto const recent = storage_.GetRecent(100);
  ASSERT_EQ(recent.size(), 0);
}

}  // namespace
