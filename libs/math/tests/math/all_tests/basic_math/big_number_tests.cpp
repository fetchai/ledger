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

#include "core/byte_array/encoders.hpp"
#include "vectorise/uint/uint.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

using namespace fetch::vectorise;
using namespace fetch::byte_array;

TEST(big_number_gtest, elementary_left_shift)
{
  UInt<256> n1(0);
  // testing elementary left shifting
  n1 = 3;
  EXPECT_EQ(3, n1[0]);
  n1 <<= 8;
  EXPECT_EQ(0, n1[0]);
  EXPECT_EQ(3, n1[1]);
  n1 <<= 7;
  EXPECT_EQ(0, n1[0]);
  EXPECT_EQ(int(128), n1[1]);
  EXPECT_EQ(int(1), n1[2]);
}

TEST(big_number_gtest, test_incrementer_for_million_increments)
{
  UInt<256> n1(0);
  {
    for (std::size_t count = 0; count < (1ul << 12u); ++count)
    {
      union
      {
        uint32_t value;
        uint8_t  bytes[sizeof(uint32_t)];
      } c;
      for (std::size_t i = 0; i < sizeof(uint32_t); ++i)
      {
        c.bytes[i] = n1[i + 1];
      }
      EXPECT_EQ(count, c.value);
      for (std::size_t i = 0; i < (1ul << 8u); ++i)
      {
        EXPECT_EQ(int(n1[0]), i);
        ++n1;
      }
    }
  }
}

TEST(big_number_gtest, testing_comparisons)
{
  UInt<256> a(0), b(0);
  for (std::size_t count = 0; count < (1ul << 8u); ++count)
  {
    EXPECT_EQ(a, b);
    for (std::size_t i = 0; i < (1ul << 8u) / 2; ++i)
    {
      ++a;
      EXPECT_LT(b, a);
    }
    for (std::size_t i = 0; i < (1ul << 8u) / 2; ++i)
    {
      EXPECT_LT(b, a);
      ++b;
    }
    EXPECT_EQ(a, b);
    for (std::size_t i = 0; i < (1ul << 8u) / 2; ++i)
    {
      ++b;
      EXPECT_GT(b, a);
    }
    for (std::size_t i = 0; i < (1ul << 8u) / 2; ++i)
    {
      EXPECT_GT(b, a);
      ++a;
    }
  }
}
