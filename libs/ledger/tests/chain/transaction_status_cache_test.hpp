#pragma once
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

#include "core/byte_array/byte_array.hpp"
#include "core/digest.hpp"
#include "core/random/lcg.hpp"

#include "gtest/gtest.h"

class TransactionStatusCacheTest : public ::testing::Test
{
public:
  using Digest = fetch::Digest;

  Digest GenerateDigest()
  {
    using fetch::byte_array::ByteArray;

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

protected:
  using Rng     = fetch::random::LinearCongruentialGenerator;
  using RngWord = Rng::RandomType;

  Rng rng_{};
};
