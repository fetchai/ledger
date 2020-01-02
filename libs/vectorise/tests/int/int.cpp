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

#include "core/byte_array/encoders.hpp"
#include "vectorise/uint/int.hpp"

#include "gmock/gmock.h"

#include <cstddef>
#include <cstdint>

using namespace fetch::vectorise;
using namespace fetch::byte_array;

TEST(Int_gtest, elementary_left_shift)
{
  Int<256> n1(0ll);
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

  Int<512> n2(UINT_MAX);
  n2 <<= 63;
}

TEST(Int_gtest, incrementer_tests)
{
  // testing incrementer for a few thousand increments close to the edges of the 64-bit container
  Int<256> n1(ULONG_MAX - 100);
  for (std::size_t count = ULONG_MAX - 100; count < ULONG_MAX; ++count)
  {
    union
    {
      uint64_t value;
      uint8_t  bytes[sizeof(uint64_t)];
    } c{};
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

TEST(Int_gtest, decrementer_tests)
{
  Int<256> n1{ULONG_MAX};
  n1 <<= 192;
  std::cout << "n1 = " << n1 << std::endl;
  // testing incrementer for a few thousand increments close to the edges of the 64-bit container
  for (std::size_t count = 0; count < 100; ++count)
  {
    --n1;
    std::cout << "n1 = " << n1 << std::endl;
  }
  std::cout << "n1 = " << n1 << std::endl;
  EXPECT_EQ(n1.ElementAt(0), ULONG_MAX - 99);
  EXPECT_EQ(n1.ElementAt(1), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(2), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(3), ULONG_MAX - 1);
}

TEST(Int_gtest, addition_tests)
{
  Int<256> n1{ULONG_MAX};
  Int<256> n2{ULONG_MAX};
  n1 <<= 32;

  Int<256> n3 = n1 + n2;

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

  std::cout << "n1 = " << std::string(n1) << std::endl;
  std::cout << "n3 = " << std::string(n3) << std::endl;
  n3 = -n3;
  std::cout << "n3 = " << std::string(n3) << std::endl;
  n3 += n1;
  std::cout << "n3 = " << std::string(n3) << std::endl;

  Int<256> n4{1};
  std::cout << "n4 = " << std::string(n4) << std::endl;
  std::cout << "n4 = " << std::string(-n4) << std::endl;
  n4 -= 1;
  std::cout << "n4 = " << std::string(n4) << std::endl;
  n4 -= 1;
  std::cout << "n4 = " << std::string(n4) << std::endl;
  std::cout << std::hex << +1 << std::endl;
  std::cout << std::hex << -1 << std::endl;
  std::cout << std::hex << (+1 - 2) << std::endl;
}

TEST(Int_gtest, subtraction_tests)
{
  Int<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000000ffffffff;
  n1.ElementAt(2) = 0x1;
  Int<256> n2{ULONG_MAX};
  n2 <<= 64;
  ++n2;
  Int<256> n3 = n1 - n2;

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

  Int<256> n4;
  n4.ElementAt(0) = 0x0000000000000000;
  n4.ElementAt(1) = 0xfffffffffffffff6;
  n4.ElementAt(2) = 0xffffffffffffffff;
  n4.ElementAt(3) = 0xffffffffffffffff;
  std::cout << "n4 = " << n4 << std::endl;
  Int<256> n5;
  n5.ElementAt(0) = 0x169281dbfff40000;
  n5.ElementAt(1) = 0xfffffffffffffff6;
  n5.ElementAt(2) = 0xffffffffffffffff;
  n5.ElementAt(3) = 0xffffffffffffffff;
  std::cout << "n5 = " << n5 << std::endl;

  Int<256> n6 = n4 - n5;
  std::cout << "n6 = " << n6 << std::endl;
}

TEST(Int_gtest, multiplication_tests)
{
  Int<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000000ffffffff;
  n1.ElementAt(2) = 0x1;
  Int<256> n2{ULONG_MAX};
  n2 <<= 64;
  ++n2;
  Int<256> n3 = n1 * n2;
  EXPECT_EQ(n3.ElementAt(0), 0xffffffff00000000);
  EXPECT_EQ(n3.ElementAt(1), 0x00000001ffffffff);
  EXPECT_EQ(n3.ElementAt(2), 0xfffffffe00000001);
  EXPECT_EQ(n3.ElementAt(3), 0x00000000fffffffe);
  Int<256> n4;
  n4.ElementAt(0) = 0x72f4a7ca9e22b75b;
  n4.ElementAt(1) = 0x00000001264eb563;
  n4.ElementAt(2) = 0;
  n4.ElementAt(3) = 0;
  Int<256> n5(0xdeadbeefdeadbeef);
  n4 *= n5;
  EXPECT_EQ(n4.ElementAt(0), 0x38fdb7f338fdb7f5);
  EXPECT_EQ(n4.ElementAt(1), 0xfffffffeffffffff);
  EXPECT_EQ(n4.ElementAt(2), 0x00000000fffffffe);
  EXPECT_EQ(n4.ElementAt(3), 0);
}

TEST(Int_gtest, division_tests)
{
  Int<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000001ffffffff;
  n1.ElementAt(2) = 0xfffffffe00000001;
  n1.ElementAt(3) = 0x00000000fffffffe;
  Int<256> n2{ULONG_MAX};
  n2 <<= 64;
  Int<256> n3 = n1 / n2;
  EXPECT_EQ(n3.ElementAt(0), 0xffffffff00000000);
  EXPECT_EQ(n3.ElementAt(1), 0x00000000fffffffe);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0);
  n3 <<= 64;
  Int<256> n4{n3};
  n3 /= Int<256>{0xdeadbeefdeadbeef};
  EXPECT_EQ(n3.ElementAt(0), 0x72f4a7ca9e22b75b);
  EXPECT_EQ(n3.ElementAt(1), 0x00000001264eb563);
  EXPECT_EQ(n3.ElementAt(2), 0);
  EXPECT_EQ(n3.ElementAt(3), 0);

  n4 %= Int<256>{0xdeadbeefdeadbeef};
  EXPECT_EQ(n4.ElementAt(0), 0xc702480cc702480b);
  EXPECT_EQ(n4.ElementAt(1), 0);
  EXPECT_EQ(n4.ElementAt(2), 0);
  EXPECT_EQ(n4.ElementAt(3), 0);
}

TEST(Int_gtest, msb_lsb_tests)
{
  Int<256> n1;
  n1.ElementAt(0) = 0xffffffff00000000;
  n1.ElementAt(1) = 0x00000001ffffffff;
  n1.ElementAt(2) = 0xfffffffe00000001;
  n1.ElementAt(3) = 0x00000000fffffffe;

  EXPECT_EQ(n1.msb(), 32);
  EXPECT_EQ(n1.lsb(), 32);
  n1 <<= 17;
  EXPECT_EQ(n1.msb(), 15);
  EXPECT_EQ(n1.lsb(), 49);
  n1 >>= 115;
  std::cout << n1 << std::endl;
  EXPECT_EQ(n1.msb(), 130);
  EXPECT_EQ(n1.lsb(), 30);
}

TEST(Int_gtest, left_shift_tests)
{
  Int<256> n2{ULONG_MAX};
  Int<256> n3{ULONG_MAX};
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

TEST(Int_gtest, right_shift_tests)
{
  Int<256> n1{ULONG_MAX};
  n1 <<= 192;
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), 0);
  EXPECT_EQ(n1.ElementAt(2), 0);
  EXPECT_EQ(n1.ElementAt(3), ULONG_MAX);
  // now number is negative
  std::cout << "n1 = " << n1 << std::endl;
  n1 >>= 64;
  std::cout << "n1 = " << n1 << std::endl;
  // shifting right propagates the 'sign' bit
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), 0);
  EXPECT_EQ(n1.ElementAt(2), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(3), ULONG_MAX);
  n1 >>= 126;
  std::cout << "n1 = " << n1 << std::endl;
  // shifting right propagates the 'sign' bit again
  EXPECT_EQ(n1.ElementAt(0), 0);
  EXPECT_EQ(n1.ElementAt(1), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(2), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(3), ULONG_MAX);
  n1 >>= 65;
  // and again
  std::cout << "n1 = " << n1 << std::endl;
  EXPECT_EQ(n1.ElementAt(0), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(1), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(2), ULONG_MAX);
  EXPECT_EQ(n1.ElementAt(3), ULONG_MAX);

  Int<256> n2{ULONG_MAX};
  n2 <<= 128;
  // number is just a large positive
  n2 >>= 64;
  std::cout << "n2 = " << n2 << std::endl;
  // there is no 'sign' bit to propagate
  EXPECT_EQ(n2.ElementAt(0), 0);
  EXPECT_EQ(n2.ElementAt(1), ULONG_MAX);
  EXPECT_EQ(n2.ElementAt(2), 0);
  EXPECT_EQ(n2.ElementAt(3), 0);
  n2 >>= 126;
  std::cout << "n2 = " << n2 << std::endl;
  // again
  EXPECT_EQ(n2.ElementAt(0), 3);
  EXPECT_EQ(n2.ElementAt(1), 0);
  EXPECT_EQ(n2.ElementAt(2), 0);
  EXPECT_EQ(n2.ElementAt(3), 0);
}

TEST(Int_gtest, testing_comparisons)
{
  Int<256> a(0u), b(0u);
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
