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
#include "gmock/gmock.h"
#include "vectorise/uint/uint.hpp"

#include <cstddef>
#include <cstdint>

namespace {

using namespace fetch;
using namespace fetch::memory;
using namespace fetch::vectorise;
using namespace fetch::byte_array;

using testing::HasSubstr;

using UInt72 = UInt<72>;
constexpr UInt72::WideType UInt72_WideType_Max{~UInt72::WideType{0}};

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

TEST(big_number_gtest, uint256_multiplication_overflow_test)
{
  UInt<256> n{UInt<256>::max};
  n.ElementAt(UInt<256>::WIDE_ELEMENTS - 1) = ~UInt<256>::WideType{0};
  n *= 2ull;
  EXPECT_EQ(n, UInt<256>::max - 1ull);
  EXPECT_EQ(n.ElementAt(0), (~UInt<256>::WideType{0}) * 2);
  EXPECT_EQ(n.ElementAt(1), ~UInt<256>::WideType{0});
  EXPECT_EQ(n.ElementAt(2), ~UInt<256>::WideType{0});
  EXPECT_EQ(n.ElementAt(3), ~UInt<256>::WideType{0});
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

  EXPECT_EQ(n1.msb(), 223);
  EXPECT_EQ(n1.lsb(), 32);
  n1 <<= 17;
  EXPECT_EQ(n1.msb(), 223 + 17);
  EXPECT_EQ(n1.lsb(), 32 + 17);
  n1 >>= 114;
  EXPECT_EQ(n1.msb(), 223 + 17 - 114);
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
  UInt<bits> shall_pass{ConstByteArray(bits / 8), platform::Endian::LITTLE};

  bool exception_thrown = false;
  try
  {
    UInt<bits> shall_throw{ConstByteArray(bits / 8 + 1), platform::Endian::LITTLE};
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
  UInt72 const inv{~UInt72::_0};
  EXPECT_EQ(UInt72::max, inv);

  constexpr UInt72::WideType wide_element_0{0xF0F0F0F0F0F0F0F0ull};
  constexpr UInt72::WideType wide_element_1{0xF0ull};

  UInt72 const val{wide_element_0, wide_element_1};
  UInt72 const expected_inv_val{~wide_element_0, UInt72::WideType{0x0Full}};

  EXPECT_EQ(expected_inv_val, ~val);
}

TEST(big_number_gtest, test_default_constructor)
{
  UInt72 const def;
  ASSERT_EQ(def.ElementAt(0), 0u);
  ASSERT_EQ(def.ElementAt(1), 0u);
}

TEST(big_number_gtest, test_zero)
{
  EXPECT_EQ(UInt72::_0.ElementAt(0), 0u);
  EXPECT_EQ(UInt72::_0.ElementAt(1), 0u);
}

TEST(big_number_gtest, test_issue_1383_overflow_zero_with_residual_bits)
{
  UInt72 x{UInt72::_0};
  x.ElementAt(1) = ~UInt72::WideType{0xFFull};

  ASSERT_EQ(UInt72::_0, x);
  EXPECT_EQ(UInt72::_0, x + x);
}

TEST(big_number_gtest, test_issue_1383_overflow_zero_one_with_residual_bits)
{
  UInt72 x0{UInt72::_0};
  x0.ElementAt(1) = ~UInt72::WideType{0xFFull};
  UInt72 x1{UInt72::_1};
  x1.ElementAt(1) = ~UInt72::WideType{0xFFull};

  ASSERT_EQ(UInt72::_0, x0);
  ASSERT_EQ(UInt72::_1, x1);

  EXPECT_EQ(UInt72::_1, x0 + x1);
  EXPECT_EQ(UInt72::_0, x0 / x1);
  EXPECT_EQ(UInt72::_1, UInt72::_1 / x1);
  EXPECT_EQ(UInt72::_1, x1 / UInt72::_1);
}

TEST(big_number_gtest, test_issue_1383_overflow_division_2)
{
  UInt72 x2{2ull};
  x2.ElementAt(1) = ~UInt72::WideType{0xFFull};
  UInt72 x4{4ull};
  x4.ElementAt(1) = ~UInt72::WideType{0xFFull};

  ASSERT_EQ(UInt72{2ull}, x2);
  ASSERT_EQ(UInt72{4ull}, x4);

  EXPECT_EQ(UInt72{2ull}, x4 / x2);
  EXPECT_EQ(UInt72{6ull}, x4 + x2);
  EXPECT_EQ(UInt72{2ull}, x4 - x2);
  // EXPECT_EQ(UInt72{0ull}, x4 % x2);
}

TEST(big_number_gtest, test_issue_1383_max)
{
  UInt72 max_WITH_residual_bits;
  max_WITH_residual_bits.ElementAt(0) = UInt72_WideType_Max;
  max_WITH_residual_bits.ElementAt(1) = UInt72_WideType_Max;

  // Verify that elements have been set as expected:
  ASSERT_EQ(~UInt72::WideType{0}, max_WITH_residual_bits.ElementAt(0));
  ASSERT_EQ(~UInt72::WideType{0}, max_WITH_residual_bits.ElementAt(1));

  // Verify that max value compares equal to the one with residual bits SET
  EXPECT_EQ(max_WITH_residual_bits, UInt72{UInt72::max});

  // Verify that max value compares equal to the one with residual bits *UN*set
  UInt72 reference_NO_residual_bits;
  reference_NO_residual_bits.ElementAt(0) = UInt72_WideType_Max;
  reference_NO_residual_bits.ElementAt(1) = UInt72::WideType{0xFFull};
  EXPECT_EQ(reference_NO_residual_bits, UInt72{UInt72::max});
}

TEST(big_number_gtest, test_issue_1383_test_max_shifted_right)
{
  UInt72 max_WITH_residual_bits;
  max_WITH_residual_bits.ElementAt(0) = UInt72_WideType_Max;
  max_WITH_residual_bits.ElementAt(1) = UInt72_WideType_Max;

  UInt72 expected_max_shifted;
  expected_max_shifted.ElementAt(0) = UInt72_WideType_Max;
  expected_max_shifted.ElementAt(1) = UInt72::WideType{0x7Full};

  EXPECT_EQ(expected_max_shifted, max_WITH_residual_bits >>= 1ull);
}

TEST(big_number_gtest, test_issue_1383_with_bitshift_oper)
{
  UInt72 n1{1u};

  // Scenario where bit-shift does *NOT* go over UInt72::UINT_SIZE boundary
  ASSERT_EQ(n1, 1u);
  n1 <<= UInt72::UINT_SIZE - 1;
  n1 >>= UInt72::UINT_SIZE - 1;
  ASSERT_EQ(n1, 1u);

  // Scenario where bit-shift *GOES* over UInt72::UINT_SIZE boundary
  n1 <<= UInt72::UINT_SIZE;
  n1 >>= UInt72::UINT_SIZE;
  EXPECT_EQ(n1, 0u);
}

TEST(big_number_gtest, test_issue_1383_overflow_with_bitshift_oper_2)
{
  UInt72 x;
  x.ElementAt(0) = UInt72_WideType_Max;
  x.ElementAt(1) = UInt72_WideType_Max;

  x >>= UInt72::UINT_SIZE - 1;
  EXPECT_EQ(x, 1u);
}

TEST(big_number_gtest, test_issue_1383_overflow_with_plus_oper)
{
  UInt72 x{UInt72::max};

  x += 1u;
  ASSERT_EQ(x, 0u);

  x >>= 1u;
  EXPECT_EQ(x, 0u);
}

TEST(big_number_gtest, test_issue_1383_division_oper)
{
  UInt72 x;
  x.ElementAt(0) = UInt72_WideType_Max;
  x.ElementAt(1) = UInt72_WideType_Max;

  x /= 2ull;
  EXPECT_EQ(UInt72{UInt72::max} >>= 1, x);
  EXPECT_EQ(UInt72(UInt72_WideType_Max, UInt72::WideType{0x7Full}), x);
}

TEST(big_number_gtest, test_issue_1383_overflow_with_plus)
{
  UInt72 x;
  x.ElementAt(0) = UInt72_WideType_Max;
  x.ElementAt(1) = UInt72_WideType_Max;

  x += 10ull;
  EXPECT_EQ(UInt72{9ull}, x);
}

}  // namespace
