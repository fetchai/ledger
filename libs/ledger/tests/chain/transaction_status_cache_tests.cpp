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
#include "ledger/transaction_status_cache.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace fetch {
namespace ledger {

std::ostream &operator<<(std::ostream &s, eTransactionStatus const &status)
{
  s << ToString(status);
  return s;
}

}  // namespace ledger
}  // namespace fetch

namespace {

using fetch::byte_array::ByteArray;
using fetch::ledger::TransactionStatusCache;
using fetch::random::LinearCongruentialGenerator;
using fetch::ledger::eTransactionStatus;
using fetch::ledger::ToString;
using fetch::ledger::Digest;

using StatusCachePtr = std::unique_ptr<TransactionStatusCache>;
using Clock          = TransactionStatusCache::Clock;
using Timepoint      = TransactionStatusCache::Timepoint;
using RngWord        = LinearCongruentialGenerator::RandomType;

class TransactionStatusCacheTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    cache_ = std::make_unique<TransactionStatusCache>();
  }

  void TearDown() override
  {
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

  StatusCachePtr              cache_{};
  LinearCongruentialGenerator rng_{};
};

TEST_F(TransactionStatusCacheTests, CheckBasicUsage)
{
  auto tx1 = GenerateDigest();
  auto tx2 = GenerateDigest();
  auto tx3 = GenerateDigest();

  cache_->Update(tx1, eTransactionStatus::PENDING);
  cache_->Update(tx2, eTransactionStatus::MINED);
  cache_->Update(tx3, eTransactionStatus::EXECUTED);

  EXPECT_EQ(eTransactionStatus::PENDING, cache_->Query(tx1).status);
  EXPECT_EQ(eTransactionStatus::MINED, cache_->Query(tx2).status);
  EXPECT_EQ(eTransactionStatus::EXECUTED, cache_->Query(tx3).status);
}

TEST_F(TransactionStatusCacheTests, CheckPruning)
{
  auto tx1 = GenerateDigest();
  auto tx2 = GenerateDigest();
  auto tx3 = GenerateDigest();

  cache_->Update(tx1, eTransactionStatus::PENDING);
  cache_->Update(tx2, eTransactionStatus::MINED);

  EXPECT_EQ(eTransactionStatus::PENDING, cache_->Query(tx1).status);
  EXPECT_EQ(eTransactionStatus::MINED, cache_->Query(tx2).status);

  Timepoint const future_time_point = Clock::now() + std::chrono::hours{25};
  cache_->Update(tx3, eTransactionStatus::EXECUTED, {}, future_time_point);

  EXPECT_EQ(eTransactionStatus::UNKNOWN, cache_->Query(tx1).status);
  EXPECT_EQ(eTransactionStatus::UNKNOWN, cache_->Query(tx2).status);
  EXPECT_EQ(eTransactionStatus::EXECUTED, cache_->Query(tx3).status);
}

}  // namespace
