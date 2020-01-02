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

#include "chain/transaction.hpp"
#include "chain/transaction_builder.hpp"
#include "ledger/storage_unit/transaction_memory_pool.hpp"
#include "transaction_generator.hpp"

#include <vector>

#include "gtest/gtest.h"

namespace {

using fetch::ledger::TransactionMemoryPool;

class TransactionMemPoolTests : public ::testing::Test
{
protected:
  TransactionGenerator  tx_gen_;
  TransactionMemoryPool memory_pool_;
};

TEST_F(TransactionMemPoolTests, SimpleCheck)
{
  auto const txs = tx_gen_.GenerateRandomTxs(5);

  for (std::size_t i = 0; i < txs.size(); ++i)
  {
    ASSERT_EQ(memory_pool_.GetCount(), i);

    // check to see if the previous transaction has been added to the memory pool
    for (std::size_t j = 0; j < i; ++j)
    {
      ASSERT_TRUE(memory_pool_.Has(txs.at(j)->digest()));
    }

    // check to see if the pending transaction are NOT present
    for (std::size_t j = i; j < txs.size(); ++j)
    {
      ASSERT_FALSE(memory_pool_.Has(txs.at(j)->digest()));
    }

    // add the current transaction to the pool
    memory_pool_.Add(*txs.at(i));

    ASSERT_EQ(memory_pool_.GetCount(), i + 1);

    // check to see if the previous transaction has been added to the memory pool
    for (std::size_t j = 0; j <= i; ++j)
    {
      ASSERT_TRUE(memory_pool_.Has(txs.at(j)->digest())) << "J: " << j;
    }

    // check to see if the pending transaction are NOT present
    for (std::size_t j = i + 1; j < txs.size(); ++j)
    {
      ASSERT_FALSE(memory_pool_.Has(txs.at(j)->digest()));
    }
  }
}

}  // namespace
