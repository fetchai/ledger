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

using namespace fetch::vectorise;
using namespace fetch::byte_array;
TEST(big_number_gtest, elemntary_left_shift)
{
  UInt<256> n1(0);
  // testing elementary left shifting
  n1 = 3;
  std::cout << "n1 = " << std::string(n1) << std::endl;
  EXPECT_EQ(3, n1[0]);
  n1 <<= 8;
  std::cout << "n1 = " << std::string(n1) << std::endl;
  EXPECT_EQ(0, n1[0]);
  EXPECT_EQ(3, n1[1]);
  n1 <<= 7;
  std::cout << "n1 = " << std::string(n1) << std::endl;
  EXPECT_EQ(0, n1[0]);
  EXPECT_EQ(int(128), n1[1]);
  EXPECT_EQ(int(1), n1[2]);
  n1 <<= 35;
  std::cout << "n1 = " << std::string(n1) << std::endl;
  n1 <<= 58;
  std::cout << "n1 = " << std::string(n1) << std::endl;
  n1 <<= 35;
  std::cout << "n1 = " << std::string(n1) << std::endl;
  n1 <<= 58;
  std::cout << "n1 = " << std::string(n1) << std::endl;

  UInt<512> n2(INT_MAX);
  n2 <<= 63;
  std::cout << "n2 = " << std::string(n2) << std::endl;
}
TEST(big_number_gtest, incrementer_tests)
{
  // testing incrementer for a few thousand increments close to the edges of the 64-bit container
  UInt<256> n1(ULONG_MAX - 100);
  std::cout << "LONG_MAX/2 = " << std::hex << ULONG_MAX - 100 << std::endl;
  std::cout << "n1 = " << std::string(n1) << std::endl;
  {
    for (std::size_t count = ULONG_MAX - 100; count < ULONG_MAX; ++count)
    {
      union
      {
        uint64_t value;
        uint8_t  bytes[sizeof(uint64_t)];
      } c;
      for (std::size_t i = 0; i < sizeof(uint64_t); ++i)
      {
        c.bytes[i] = n1[i];
      }
      std::cout << "count = " << count << std::endl;
      std::cout << "c.value = " << c.value << std::endl;
      std::cout << "n1 = " << std::string(n1) << std::endl;
      EXPECT_EQ(count, c.value);
      std::cout << "n1 = " << std::string(n1) << std::endl;
      EXPECT_EQ(n1.elementAt(0), count);
      ++n1;
    }
    std::cout << "n1 = " << std::string(n1) << std::endl;
    UInt<256> n2{n1};
    ++n1;
    EXPECT_EQ(n1.elementAt(0), 0);
    EXPECT_EQ(n1.elementAt(1), 1);
    n2 <<= 63;
    std::cout << "n2 = " << std::string(n2) << std::endl;
  }
}
TEST(big_number_gtest, testing_comparisons)
{
  UInt<256> a(0), b(0);
  for (std::size_t count = 0; count < (1 << 8); ++count)
  {
    EXPECT_EQ(a, b);
    for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
    {
      ++a;
      EXPECT_LT(b, a);
    }
    for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
    {
      EXPECT_LT(b, a);
      ++b;
    }
    EXPECT_EQ(a, b);
    for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
    {
      ++b;
      EXPECT_GT(b, a);
    }
    for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
    {
      EXPECT_GT(b, a);
      ++a;
    }
  }
}
