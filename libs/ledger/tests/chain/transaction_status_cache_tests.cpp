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

#include "time_based_transaction_status_cache.hpp"
#include "transaction_status_cache_test.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::Digest;
using fetch::ledger::ContractExecutionResult;
using fetch::ledger::ContractExecutionStatus;
using fetch::ledger::TimeBasedTransactionStatusCache;
using fetch::ledger::TransactionStatus;

using AdjustableClockPtr = fetch::moment::AdjustableClockPtr;
using Timestamp          = fetch::moment::ClockInterface::Timestamp;

class TransactionStatusCacheTests : public TransactionStatusCacheTest
{
protected:
  AdjustableClockPtr              clock_{fetch::moment::CreateAdjustableClock("tx-status")};
  TimeBasedTransactionStatusCache cache_{};
};

TEST_F(TransactionStatusCacheTests, CheckBasicUpdate)
{
  auto tx1 = GenerateDigest();
  auto tx2 = GenerateDigest();
  auto tx3 = GenerateDigest();

  // make updates for a series of TXs
  cache_.Update(tx1, TransactionStatus::SUBMITTED);
  cache_.Update(tx2, TransactionStatus::PENDING);
  cache_.Update(tx3, TransactionStatus::MINED);

  ASSERT_EQ(TransactionStatus::SUBMITTED, cache_.Query(tx1).status);
  ASSERT_EQ(TransactionStatus::PENDING, cache_.Query(tx2).status);
  ASSERT_EQ(TransactionStatus::MINED, cache_.Query(tx3).status);

  cache_.Update(tx1, TransactionStatus::PENDING);
  cache_.Update(tx2, TransactionStatus::MINED);

  EXPECT_EQ(TransactionStatus::PENDING, cache_.Query(tx1).status);
  EXPECT_EQ(TransactionStatus::MINED, cache_.Query(tx2).status);
  EXPECT_EQ(TransactionStatus::MINED, cache_.Query(tx3).status);
}

TEST_F(TransactionStatusCacheTests, CheckTxStatusUpdateFailsForExecutedStatus)
{
  auto tx1 = GenerateDigest();

  cache_.Update(tx1, TransactionStatus::PENDING);
  ASSERT_EQ(TransactionStatus::PENDING, cache_.Query(tx1).status);

  ASSERT_THROW(cache_.Update(tx1, TransactionStatus::EXECUTED), std::runtime_error);
}

TEST_F(TransactionStatusCacheTests, CheckUpdateForContractExecutionResult)
{
  auto tx1 = GenerateDigest();

  cache_.Update(tx1, TransactionStatus::PENDING);
  ASSERT_EQ(TransactionStatus::PENDING, cache_.Query(tx1).status);

  ContractExecutionResult const expected_result{
      ContractExecutionStatus::INEXPLICABLE_FAILURE, 1, 2, 3, 4, -2};

  cache_.Update(tx1, expected_result);

  auto const received_result{cache_.Query(tx1)};
  EXPECT_EQ(TransactionStatus::EXECUTED, received_result.status);
  EXPECT_EQ(expected_result.status, received_result.contract_exec_result.status);
  EXPECT_EQ(expected_result.return_value, received_result.contract_exec_result.return_value);
  EXPECT_EQ(expected_result.fee, received_result.contract_exec_result.fee);
  EXPECT_EQ(expected_result.charge_limit, received_result.contract_exec_result.charge_limit);
  EXPECT_EQ(expected_result.charge_rate, received_result.contract_exec_result.charge_rate);
  EXPECT_EQ(expected_result.charge, received_result.contract_exec_result.charge);
}

TEST_F(TransactionStatusCacheTests, CheckPruning)
{
  auto tx1 = GenerateDigest();
  auto tx2 = GenerateDigest();
  auto tx3 = GenerateDigest();

  cache_.Update(tx1, TransactionStatus::PENDING);
  cache_.Update(tx2, TransactionStatus::MINED);

  ASSERT_EQ(TransactionStatus::PENDING, cache_.Query(tx1).status);
  ASSERT_EQ(TransactionStatus::MINED, cache_.Query(tx2).status);

  // advance our clock by other the cache threshold
  clock_->Advance(std::chrono::hours{25});

  cache_.Update(tx3, TransactionStatus::SUBMITTED);

  EXPECT_EQ(TransactionStatus::UNKNOWN, cache_.Query(tx1).status);
  EXPECT_EQ(TransactionStatus::UNKNOWN, cache_.Query(tx2).status);
  EXPECT_EQ(TransactionStatus::SUBMITTED, cache_.Query(tx3).status);
}

}  // namespace
