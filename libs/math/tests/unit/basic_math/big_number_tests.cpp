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
#include "gmock/gmock.h"
#include "vectorise/uint/uint.hpp"

#include <cstddef>
#include <cstdint>

using namespace fetch::vectorise;
using namespace fetch::byte_array;

using testing::HasSubstr;

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

TEST(big_number_gtest, log_tests)
{
  for (const auto argument :
       {uint64_t(1), uint64_t(64), uint64_t(65536), uint64_t(std::numeric_limits<uint64_t>::max())})
  {
    UInt<256> n256(argument);

    const auto result   = Log(n256);
    const auto expected = std::log(argument);

    EXPECT_NEAR(result, expected, std::numeric_limits<double>::epsilon());
  }
}

TEST(big_number_gtest, to_double_tests)
{
  EXPECT_NEAR(ToDouble(UInt<256>(uint64_t(0))), 0., std::numeric_limits<double>::epsilon());

  static constexpr double MAX_UINT256_VALUE = 1.15792089237316e+77;
  for (auto seed : {1., 10., 100., 1000., 10000.})
  {
    UInt<256> number(static_cast<uint64_t>(seed));

    for (size_t i = 0; i < number.ELEMENTS; ++i)
    {
      const double result   = ToDouble(number);
      const double expected = seed * pow(2, i * number.ELEMENT_SIZE);
      if (expected < MAX_UINT256_VALUE)
      {
        EXPECT_NEAR(result, expected, std::numeric_limits<double>::epsilon());
      }
      else
      {
        EXPECT_NE(result, expected);
      }
      number <<= number.ELEMENT_SIZE;
    }
  }
}

template <typename LongUInt>
void TestTrimmedWideSize()
{
  EXPECT_EQ(0, LongUInt(uint64_t(0)).TrimmedWideSize());
  LongUInt number(uint64_t(0x80));
  for (size_t i = 0; i < number.ELEMENTS; i++)
  {
    const auto expected_trimmed_size = i / (number.ELEMENTS / number.WIDE_ELEMENTS) + 1;
    EXPECT_EQ(expected_trimmed_size, number.TrimmedWideSize());
    number <<= number.ELEMENT_SIZE;
  }
}

TEST(big_number_gtest, trimmed_size_tests)
{
  TestTrimmedWideSize<UInt<32>>();
  TestTrimmedWideSize<UInt<64>>();
  TestTrimmedWideSize<UInt<128>>();
  TestTrimmedWideSize<UInt<256>>();
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
  UInt<256> n5(0xdeadbeefdeadbeef);
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

  UInt<256> n5{ULONG_MAX};
  UInt<256> n6{ULONG_MAX};
  n5 = n5 / 1ull;
  EXPECT_EQ(n5, n6);

  n5 = n4 / n5;
  EXPECT_EQ(n5.ElementAt(0), 0);
  EXPECT_EQ(n5.ElementAt(1), 0);
  EXPECT_EQ(n5.ElementAt(2), 0);
  EXPECT_EQ(n5.ElementAt(3), 0);
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

TEST(big_number_gtest, test_construction_from_byte_array_fails_if_too_long)
{
  constexpr std::size_t bits{256};

  // Verify that construction pass is size is <= bits/8
  UInt<bits> shall_pass{ConstByteArray(bits / 8)};

  bool exception_thrown = false;
  try
  {
    UInt<bits> shall_throw{ConstByteArray(bits / 8 + 1)};
  }
  catch (std::exception const &ex)
  {
    EXPECT_THAT(ex.what(), HasSubstr("Size of input byte array is bigger than"));
    exception_thrown = true;
  }

  EXPECT_TRUE(exception_thrown);
}

TEST(big_number_gtest, test_bit_inverse)
{
  using UIntT = UInt<72>;

  UIntT const inv{~UIntT::_0};
  EXPECT_EQ(inv.ElementAt(0), 0xffffffffffffffff);
  EXPECT_EQ(inv.ElementAt(1), 0xff);
  EXPECT_EQ(UIntT::max, inv);

  // UIntT val{};
  // val.ElementAt(0) = 0xf0f0f0f0f0f0f0f0;
  // val.ElementAt(1) = 0xf0;
  UIntT val{UIntT::WideType{0xf0f0f0f0f0f0f0f0ull}, UIntT::WideType{0xf0ull}};

  auto const inv2{~val};

  ASSERT_EQ(inv2.ElementAt(0), 0x0f0f0f0f0f0f0f0f);
  ASSERT_EQ(inv2.ElementAt(1), 0x0f);
}

TEST(big_number_gtest, test_default_constructor)
{
  using UIntT = UInt<72>;

  UIntT const def;
  ASSERT_EQ(def.ElementAt(0), 0u);
  ASSERT_EQ(def.ElementAt(1), 0u);
}

TEST(big_number_gtest, test_zero)
{
  using UIntT = UInt<72>;

  EXPECT_EQ(UIntT::_0.ElementAt(0), 0u);
  EXPECT_EQ(UIntT::_0.ElementAt(1), 0u);
}

// TODO(issue 1383): Enable test when issue is resolved
TEST(big_number_gtest, DISABLED_test_issue_1383_demo_with_bitshift_oper)
{
  using UIntT = UInt<72>;

  UIntT n1{1u};

  // Scenario where bit-shift does *NOT* go over UIntT::UINT_SIZE boundary
  ASSERT_EQ(n1, 1u);
  n1 <<= UIntT::UINT_SIZE - 1;
  n1 >>= UIntT::UINT_SIZE - 1;
  ASSERT_EQ(n1, 1u);

  // Scenario where bit-shift *GOES* over UIntT::UINT_SIZE boundary
  n1 <<= UIntT::UINT_SIZE;
  n1 >>= UIntT::UINT_SIZE;

  EXPECT_EQ(n1, 0u);
}

// TODO(issue 1383): Enable test when issue is resolved
TEST(big_number_gtest, DISABLED_test_issue_1383_demo_overflow_with_plus_minus_oper)
{
  using UIntT = UInt<72>;
  UIntT n1{UIntT::max};
  ASSERT_EQ(UIntT::max.ElementAt(0), ~UIntT::WideType{0});
  ASSERT_EQ(UIntT::max.ElementAt(1), 0xff);

  ASSERT_EQ(n1.ElementAt(0), ~UIntT::WideType{0});
  ASSERT_EQ(n1.ElementAt(1),
            ~UIntT::WideType{0} >>
                (UIntT::WIDE_ELEMENT_SIZE - (UIntT::UINT_SIZE % UIntT::WIDE_ELEMENT_SIZE)));

  n1 += 1u;
  n1 -= 1u;
  EXPECT_EQ(n1, 0u);
}
