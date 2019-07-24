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

#include "core/byte_array/byte_array.hpp"
#include "core/macros.hpp"
#include "core/random/lcg.hpp"
#include "ledger/transaction_status_cache_impl.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>

namespace {

using namespace fetch;
using namespace fetch::ledger;

using fetch::byte_array::ByteArray;
using fetch::random::LinearCongruentialGenerator;
using StatusCachePtr = TransactionStatusCache::ShrdPtr;
using RngWord        = LinearCongruentialGenerator::RandomType;

using testing::Return;
using testing::HasSubstr;

struct ClockSteadyClockMock;

struct ClockMock : public std::chrono::steady_clock
{
  static std::shared_ptr<ClockSteadyClockMock> mock;
  static time_point                            now() noexcept;
};
std::shared_ptr<ClockSteadyClockMock> ClockMock::mock;

struct ClockSteadyClockMock
{
  MOCK_CONST_METHOD0(now, ClockMock::time_point());
};

ClockMock::time_point ClockMock::now() noexcept
{
  return mock->now();
}

using TransactionStatusCacheForTest = TransactionStatusCacheImpl<ClockMock>;
using Clock                         = TransactionStatusCacheForTest::Clock;
using Timepoint                     = TransactionStatusCacheForTest::Timepoint;

class TransactionStatusCacheTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    clock_mock_     = std::make_shared<ClockSteadyClockMock>();
    ClockMock::mock = clock_mock_;
    EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(Timepoint::min()));
    cache_ = std::make_shared<TransactionStatusCacheForTest>();
  }

  void TearDown() override
  {
    ClockMock::mock.reset();
    clock_mock_.reset();
    cache_.reset();
  }

  Digest GenerateDigest()
  {
    static constexpr std::size_t DIGEST_BIT_LENGTH  = 256u;
    static constexpr std::size_t DIGEST_BYTE_LENGTH = DIGEST_BIT_LENGTH / 8u;
    static constexpr std::size_t RNG_WORD_SIZE      = sizeof(RngWord);
    static constexpr std::size_t NUM_WORDS          = DIGEST_BYTE_LENGTH / RNG_WORD_SIZE;

    static_assert((DIGEST_BYTE_LENGTH % RNG_WORD_SIZE) == 0, "");

    ByteArray digest;
    digest.Resize(DIGEST_BYTE_LENGTH);

    auto *digest_raw = reinterpret_cast<RngWord *>(digest.pointer());
    for (std::size_t i = 0; i < NUM_WORDS; ++i)
    {
      *digest_raw++ = rng_();
    }

    return Digest{digest};
  }

  StatusCachePtr                        cache_{};
  LinearCongruentialGenerator           rng_{};
  std::shared_ptr<ClockSteadyClockMock> clock_mock_{};
};

TEST_F(TransactionStatusCacheTests, CheckBasicUpdate)
{
  auto tx1 = GenerateDigest();
  auto tx2 = GenerateDigest();
  auto tx3 = GenerateDigest();

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(Timepoint::min()));
  cache_->Update(tx1, TransactionStatus::SUBMITTED);

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(Timepoint::min()));
  cache_->Update(tx2, TransactionStatus::PENDING);

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(Timepoint::min()));
  cache_->Update(tx3, TransactionStatus::MINED);

  ASSERT_EQ(TransactionStatus::SUBMITTED, cache_->Query(tx1).status);
  ASSERT_EQ(TransactionStatus::PENDING, cache_->Query(tx2).status);
  ASSERT_EQ(TransactionStatus::MINED, cache_->Query(tx3).status);

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(Timepoint::min()));
  cache_->Update(tx1, TransactionStatus::PENDING);

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(Timepoint::min()));
  cache_->Update(tx2, TransactionStatus::MINED);

  EXPECT_EQ(TransactionStatus::PENDING, cache_->Query(tx1).status);
  EXPECT_EQ(TransactionStatus::MINED, cache_->Query(tx2).status);
  EXPECT_EQ(TransactionStatus::MINED, cache_->Query(tx3).status);
}

TEST_F(TransactionStatusCacheTests, CheckTxStatusUpdateFailsForExecutedStatus)
{
  auto tx1 = GenerateDigest();

  Timepoint start{ClockMock::time_point::min()};

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(start));
  cache_->Update(tx1, TransactionStatus::PENDING);
  ASSERT_EQ(TransactionStatus::PENDING, cache_->Query(tx1).status);

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(start));
  bool ex_thrown{false};
  try
  {
    cache_->Update(tx1, TransactionStatus::EXECUTED);
  }
  catch (std::runtime_error const &ex)
  {
    EXPECT_THAT(ex.what(), HasSubstr("Using inappropriate method to update"));
    ex_thrown = true;
  }

  EXPECT_TRUE(ex_thrown);
}

TEST_F(TransactionStatusCacheTests, CheckUpdateForContractExecutionResult)
{
  auto tx1 = GenerateDigest();

  Timepoint start{ClockMock::time_point::min()};

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(start));
  cache_->Update(tx1, TransactionStatus::PENDING);
  ASSERT_EQ(TransactionStatus::PENDING, cache_->Query(tx1).status);

  ContractExecutionResult const expected_result{
      ContractExecutionStatus::INEXPLICABLE_FAILURE, 1, 2, 3, 4, -2};

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(start));
  cache_->Update(tx1, expected_result);

  auto const received_result{cache_->Query(tx1)};
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

  Timepoint start{ClockMock::time_point::min()};

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(start));
  cache_->Update(tx1, TransactionStatus::PENDING);

  EXPECT_CALL(*clock_mock_, now()).WillOnce(Return(start));
  cache_->Update(tx2, TransactionStatus::MINED);

  ASSERT_EQ(TransactionStatus::PENDING, cache_->Query(tx1).status);
  ASSERT_EQ(TransactionStatus::MINED, cache_->Query(tx2).status);

  Timepoint const future_time_point = start + std::chrono::hours{25};

  EXPECT_CALL(*clock_mock_, now()).WillRepeatedly(Return(future_time_point));
  cache_->Update(tx3, TransactionStatus::SUBMITTED);

  EXPECT_EQ(TransactionStatus::UNKNOWN, cache_->Query(tx1).status);
  EXPECT_EQ(TransactionStatus::UNKNOWN, cache_->Query(tx2).status);
  EXPECT_EQ(TransactionStatus::SUBMITTED, cache_->Query(tx3).status);
}

}  // namespace
