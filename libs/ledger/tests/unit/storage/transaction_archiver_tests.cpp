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

#include "ledger/storage_unit/transaction_archiver.hpp"
#include "mock_transaction_pool.hpp"
#include "mock_transaction_store.hpp"
#include "transaction_generator.hpp"

#include "gtest/gtest.h"

namespace {

using testing::_;
using testing::InSequence;
using testing::Return;
using testing::NiceMock;
using fetch::ledger::TransactionArchiver;

class TransactionArchiverTests : public ::testing::Test
{
protected:
  void CycleStateMachine()
  {
    auto state_machine = archiver_.GetStateMachine();

    for (std::size_t i = 0; i < 10; ++i)
    {
      state_machine->Execute();

      // exit the state machine when the next state is collecting again
      if (state_machine->state() == TransactionArchiver::State::COLLECTING)
      {
        return;
      }
    }

    FAIL() << "State machine never completed";
  }

  NiceMock<MockTransactionPool>  pool_;
  NiceMock<MockTransactionStore> store_;
  TransactionGenerator           tx_gen_;
  TransactionArchiver            archiver_{0, pool_, store_};
};

MATCHER_P(IsTransaction, digest, "")  // NOLINT
{
  return arg.digest() == digest;
}

TEST_F(TransactionArchiverTests, BasicCheck)
{
  auto const txs = tx_gen_.GenerateRandomTxs(5);

  // add all the transactions to the memory pool
  for (auto const &tx : txs)
  {
    pool_.pool.Add(*tx);
  }

  // one by one confirm each of the transaction and check to make sure they have
  // made it to the archive
  for (std::size_t i = 0; i < 5; ++i)
  {
    auto const current = txs[i]->digest();

    // set up expectations
    InSequence seq;
    EXPECT_CALL(pool_, Get(current, _)).Times(1);
    EXPECT_CALL(store_, Add(IsTransaction(current))).Times(1);
    EXPECT_CALL(pool_, Remove(current)).Times(1);

    // signal to the archiver that the transaction has been confirmed
    archiver_.Confirm(current);

    // run the state machine of the archiver
    CycleStateMachine();

    for (std::size_t j = 0; j <= i; ++j)
    {
      auto const verify = txs[j]->digest();

      EXPECT_TRUE(store_.pool.Has(verify));
      EXPECT_FALSE(pool_.pool.Has(verify));
    }
  }
}

TEST_F(TransactionArchiverTests, CheckRecoveryFromLookupFailure)
{
  auto const tx = tx_gen_();

  // confirm the transaction for which we know the lookup will fail
  archiver_.Confirm(tx->digest());

  {
    InSequence seq;
    EXPECT_CALL(pool_, Get(tx->digest(), _)).Times(1).WillOnce(Return(false));

    CycleStateMachine();
  }

  // ensure normal operation resumes
  auto const tx2 = tx_gen_();
  pool_.pool.Add(*tx2);

  auto const current = tx2->digest();

  // confirm the next transaction
  archiver_.Confirm(current);

  {
    // check the state machine does its job
    InSequence seq;
    EXPECT_CALL(pool_, Get(current, _)).Times(1);
    EXPECT_CALL(store_, Add(IsTransaction(current))).Times(1);
    EXPECT_CALL(pool_, Remove(current)).Times(1);

    CycleStateMachine();
  }

  EXPECT_TRUE(store_.pool.Has(current));
  EXPECT_FALSE(pool_.pool.Has(current));
}

}  // namespace
