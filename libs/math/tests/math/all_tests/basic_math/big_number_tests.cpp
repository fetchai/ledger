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
  UInt<256> n1(0ull);
  // testing elementary left shifting
  n1 = 3ull;
  EXPECT_EQ(3, n1[0]);
  n1 <<= 8;
  EXPECT_EQ(0, n1[0]);
  EXPECT_EQ(3, n1[1]);
  n1 <<= 7;
  EXPECT_EQ(0, n1[0]);
  EXPECT_EQ(int(128), n1[1]);
  EXPECT_EQ(int(1), n1[2]);
  n1 <<= 35;
  n1 <<= 58;
  n1 <<= 35;
  n1 <<= 58;

  UInt<512> n2(UINT_MAX);
  n2 <<= 63;
}
TEST(big_number_gtest, incrementer_tests)
{
  // testing incrementer for a few thousand increments close to the edges of the 64-bit container
  UInt<256> n1(ULONG_MAX - 100);
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
    EXPECT_EQ(count, c.value);
    EXPECT_EQ(n1.ElementAt(0), count);
    ++n1;
  }
  ++n1;
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), 1);
}

TEST(big_number_gtest, decrementer_tests)
{
  UInt<256> n1{ULONG_MAX};
  n1 <<= 192;
  // testing incrementer for a few thousand increments close to the edges of the 64-bit container
  for (std::size_t count = 0; count < 100; ++count)
  {
    --n1;
  }
  EXPECT_EQ(n1.ElementAt(0), ULONG_MAX - 99);
  EXPECT_EQ(n1.ElementAt(1), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(2), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(3), ULONG_MAX - 1);
}

TEST(big_number_gtest, addition_tests)
{
  UInt<256> n1{ULONG_MAX};
  UInt<256> n2{ULONG_MAX};
  n1 <<= 32;

  UInt<256> n3 = n1 + n2;

  // 0x100000000fffffffeffffffff
  EXPECT_EQ(n3.ElementAt(0), 0xfffffffeffffffff);
  EXPECT_EQ(n3.ElementAt(1), 0x100000000);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0);

  n1 <<= 32;
  ++n1;
  n3 += n1;
  EXPECT_EQ(n3.ElementAt(0), 0xffffffff00000000);
  EXPECT_EQ(n3.ElementAt(1), 0x00000000ffffffff);
  EXPECT_EQ(n3.ElementAt(2), 0x1);
  EXPECT_EQ(n3.ElementAt(3), 0);
}

TEST(big_number_gtest, subtraction_tests)
{
  UInt<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000000ffffffff;
  n1.ElementAt(2) = 0x1;
  UInt<256> n2{ULONG_MAX};
  n2 <<= 64;
  ++n2;
  UInt<256> n3 = n1 - n2;

  EXPECT_EQ(n3.ElementAt(0), 0xfffffffeffffffff);
  EXPECT_EQ(n3.ElementAt(1), 0x0000000100000000);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0);

  n2 >>= 32;
  n3 -= n2;
  EXPECT_EQ(n3.ElementAt(0), ULONG_MAX);
  EXPECT_EQ(n3.ElementAt(1), 0);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0);
}

TEST(big_number_gtest, multiplication_tests)
{
  UInt<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000000ffffffff;
  n1.ElementAt(2) = 0x1;
  UInt<256> n2{ULONG_MAX};
  n2 <<= 64;
  ++n2;
  UInt<256> n3 = n1 * n2;
  EXPECT_EQ(n3.ElementAt(0), 0xffffffff00000000);
  EXPECT_EQ(n3.ElementAt(1), 0x00000001ffffffff);
  EXPECT_EQ(n3.ElementAt(2), 0xfffffffe00000001);
  EXPECT_EQ(n3.ElementAt(3), 0x00000000fffffffe);
  UInt<256> n4;
  n4.ElementAt(0) = 0x72f4a7ca9e22b75b;
  n4.ElementAt(1) = 0x00000001264eb563;
  n4.ElementAt(2) = 0;
  n4.ElementAt(3) = 0;
  UInt<256> n5    = 0xdeadbeefdeadbeef;
  n4 *= n5;
  EXPECT_EQ(n4.ElementAt(0), 0x38fdb7f338fdb7f5);
  EXPECT_EQ(n4.ElementAt(1), 0xfffffffeffffffff);
  EXPECT_EQ(n4.ElementAt(2), 0x00000000fffffffe);
  EXPECT_EQ(n4.ElementAt(3), 0);
}

TEST(big_number_gtest, division_tests)
{
  UInt<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000001ffffffff;
  n1.ElementAt(2) = 0xfffffffe00000001;
  n1.ElementAt(3) = 0x00000000fffffffe;
  UInt<256> n2{ULONG_MAX};
  n2 <<= 64;
  UInt<256> n3 = n1 / n2;
  EXPECT_EQ(n3.ElementAt(0), 0xffffffff00000000);
  EXPECT_EQ(n3.ElementAt(1), 0x00000000fffffffe);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0);
  n3 <<= 64;
  UInt<256> n4{n3};
  n3 /= 0xdeadbeefdeadbeef;
  EXPECT_EQ(n3.ElementAt(0), 0x72f4a7ca9e22b75b);
  EXPECT_EQ(n3.ElementAt(1), 0x00000001264eb563);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0);

  n4 %= 0xdeadbeefdeadbeef;
  EXPECT_EQ(n4.ElementAt(0), 0xc702480cc702480b);
  EXPECT_EQ(n4.ElementAt(1), 0);
  EXPECT_EQ(n4.ElementAt(2), 0);
  EXPECT_EQ(n4.ElementAt(3), 0);
}

TEST(big_number_gtest, msb_lsb_tests)
{
  UInt<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000001ffffffff;
  n1.ElementAt(2) = 0xfffffffe00000001;
  n1.ElementAt(3) = 0x00000000fffffffe;

  EXPECT_EQ(n1.msb(), 32);
  EXPECT_EQ(n1.lsb(), 32);
  n1 <<= 17;
  EXPECT_EQ(n1.msb(), 15);
  EXPECT_EQ(n1.lsb(), 49);
  n1 >>= 114;
  EXPECT_EQ(n1.msb(), 129);
  EXPECT_EQ(n1.lsb(), 31);
}

TEST(big_number_gtest, left_shift_tests)
{
  UInt<256> n2{ULONG_MAX};
  UInt<256> n3{ULONG_MAX};
  n2 <<= 63;
  EXPECT_EQ(n2.ElementAt(0), 0x8000000000000000);
  EXPECT_EQ(n2.ElementAt(1), ULONG_MAX >> 1);
  n3 <<= 64;
  EXPECT_EQ(n3.ElementAt(0), 0);
  EXPECT_EQ(n3.ElementAt(1), ULONG_MAX);
  n3 <<= 126;
  EXPECT_EQ(n3.ElementAt(0), 0);
  EXPECT_EQ(n3.ElementAt(1), 0);
  EXPECT_EQ(n3.ElementAt(2), 0xc000000000000000);
  EXPECT_EQ(n3.ElementAt(3), ULONG_MAX >> 2);
  n3 <<= 65;
  EXPECT_EQ(n3.ElementAt(0), 0);
  EXPECT_EQ(n3.ElementAt(1), 0);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0x8000000000000000);
}

TEST(big_number_gtest, right_shift_tests)
{
  UInt<256> n1{ULONG_MAX};
  n1 <<= 192;
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), 0);
  EXPECT_EQ(n1.ElementAt(2), 0);
  EXPECT_EQ(n1.ElementAt(3), ULONG_MAX);
  n1 >>= 64;
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), 0);
  EXPECT_EQ(n1.ElementAt(2), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(3), 0);
  n1 >>= 126;
  EXPECT_EQ(n1.ElementAt(0), ULONG_MAX << 2);
  EXPECT_EQ(n1.ElementAt(1), 3);
  EXPECT_EQ(n1.ElementAt(2), 0);
  EXPECT_EQ(n1.ElementAt(3), 0);
  n1 >>= 65;
  EXPECT_EQ(n1.ElementAt(0), 1);
  EXPECT_EQ(n1.ElementAt(1), 0);
  EXPECT_EQ(n1.ElementAt(2), 0);
  EXPECT_EQ(n1.ElementAt(3), 0);
}

TEST(big_number_gtest, testing_comparisons)
{
  UInt<256> a(0u), b(0u);
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

TEST(big_number_gtest, test_bits_size_not_alligned_with_wide_element_array_size)
{
  UInt<272> n1{ULONG_MAX};
  n1 <<= (272 - 64);
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), 0);
  EXPECT_EQ(n1.ElementAt(2), 0);
  EXPECT_EQ(n1.ElementAt(3), 0xffffffffffff0000);
  EXPECT_EQ(n1.ElementAt(4), 0x000000000000ffff);
  n1 >>= 8;
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), 0);
  EXPECT_EQ(n1.ElementAt(2), 0);
  EXPECT_EQ(n1.ElementAt(3), 0xffffffffffffff00);
  EXPECT_EQ(n1.ElementAt(4), 0x00000000000000ff);
}
