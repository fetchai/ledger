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

#include "ledger/storage_unit/transaction_store_aggregator.hpp"
#include "mock_transaction_store.hpp"
#include "transaction_generator.hpp"

#include "gtest/gtest.h"

namespace {

using ::testing::StrictMock;
using ::testing::_;
using ::testing::Return;
using fetch::chain::Transaction;
using fetch::ledger::TransactionStoreAggregator;

class TransactionStoreAggregatorTests : public ::testing::Test
{
protected:
  StrictMock<MockTransactionStore> pool_;
  StrictMock<MockTransactionStore> store_;
  TransactionStoreAggregator       agg_{pool_, store_};
  TransactionGenerator             tx_gen_{};
};

TEST_F(TransactionStoreAggregatorTests, CheckAdd)
{
  auto const tx = tx_gen_();

  EXPECT_CALL(pool_, Add(_)).Times(1);

  agg_.Add(*tx);
}

TEST_F(TransactionStoreAggregatorTests, CheckHasInPool)
{
  auto const tx = tx_gen_();

  EXPECT_CALL(pool_, Has(tx->digest())).Times(1).WillOnce(Return(true));

  EXPECT_TRUE(agg_.Has(tx->digest()));
}

TEST_F(TransactionStoreAggregatorTests, CheckHasInStore)
{
  auto const tx = tx_gen_();

  EXPECT_CALL(pool_, Has(tx->digest())).Times(1).WillOnce(Return(false));
  EXPECT_CALL(store_, Has(tx->digest())).Times(1).WillOnce(Return(true));

  EXPECT_TRUE(agg_.Has(tx->digest()));
}

TEST_F(TransactionStoreAggregatorTests, CheckHasNotPresent)
{
  auto const tx = tx_gen_();

  EXPECT_CALL(pool_, Has(tx->digest())).Times(1).WillOnce(Return(false));
  EXPECT_CALL(store_, Has(tx->digest())).Times(1).WillOnce(Return(false));

  EXPECT_FALSE(agg_.Has(tx->digest()));
}

TEST_F(TransactionStoreAggregatorTests, CheckGetInPool)
{
  auto const tx = tx_gen_();
  pool_.pool.Add(*tx);

  EXPECT_CALL(pool_, Get(tx->digest(), _)).Times(1);

  Transaction output{};
  EXPECT_TRUE(agg_.Get(tx->digest(), output));
  EXPECT_EQ(output.digest(), tx->digest());
}

TEST_F(TransactionStoreAggregatorTests, CheckGetInStore)
{
  auto const tx = tx_gen_();
  store_.pool.Add(*tx);

  EXPECT_CALL(pool_, Get(tx->digest(), _)).Times(1).WillOnce(Return(false));
  EXPECT_CALL(store_, Get(tx->digest(), _)).Times(1);

  Transaction output{};
  EXPECT_TRUE(agg_.Get(tx->digest(), output));
  EXPECT_EQ(output.digest(), tx->digest());
}

TEST_F(TransactionStoreAggregatorTests, CheckCounts)
{
  EXPECT_CALL(pool_, GetCount()).Times(1).WillOnce(Return(256));
  EXPECT_CALL(store_, GetCount()).Times(1).WillOnce(Return(128));

  EXPECT_EQ(agg_.GetCount(), 384);
}

TEST_F(TransactionStoreAggregatorTests, CheckCountsOnlyPool)
{
  EXPECT_CALL(pool_, GetCount()).Times(1).WillOnce(Return(256));
  EXPECT_CALL(store_, GetCount()).Times(1).WillOnce(Return(0));

  EXPECT_EQ(agg_.GetCount(), 256);
}

TEST_F(TransactionStoreAggregatorTests, CheckCountsOnlyStore)
{
  EXPECT_CALL(pool_, GetCount()).Times(1).WillOnce(Return(0));
  EXPECT_CALL(store_, GetCount()).Times(1).WillOnce(Return(128));

  EXPECT_EQ(agg_.GetCount(), 128);
}

}  // namespace
