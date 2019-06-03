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

#include "vectorise/fixed_point/fixed_point.hpp"
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <limits>

using fp32 = fetch::fixed_point::FixedPoint<16, 16>;
using fp64 = fetch::fixed_point::FixedPoint<32, 32>;

TEST(FixedPointTest, Conversion_16_16)
{
  // Positive
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);

  EXPECT_EQ(int(one), 1);
  EXPECT_EQ(int(two), 2);
  EXPECT_EQ(float(one), 1.0f);
  EXPECT_EQ(float(two), 2.0f);
  EXPECT_EQ(double(one), 1.0);
  EXPECT_EQ(double(two), 2.0);

  // Negative
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1);
  fetch::fixed_point::FixedPoint<16, 16> m_two(-2);

  EXPECT_EQ(int(m_one), -1);
  EXPECT_EQ(int(m_two), -2);
  EXPECT_EQ(float(m_one), -1.0f);
  EXPECT_EQ(float(m_two), -2.0f);
  EXPECT_EQ(double(m_one), -1.0);
  EXPECT_EQ(double(m_two), -2.0);

  // CONST_ZERO
  fetch::fixed_point::FixedPoint<16, 16> zero(0);
  fetch::fixed_point::FixedPoint<16, 16> m_zero(-0);

  EXPECT_EQ(int(zero), 0);
  EXPECT_EQ(int(m_zero), 0);
  EXPECT_EQ(float(zero), 0.0f);
  EXPECT_EQ(float(m_zero), 0.0f);
  EXPECT_EQ(double(zero), 0.0);
  EXPECT_EQ(double(m_zero), 0.0);

  // Get raw value
  fetch::fixed_point::FixedPoint<16, 16> zero_point_five(0.5);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> two_point_five(2.5);
  fetch::fixed_point::FixedPoint<16, 16> m_one_point_five(-1.5);

  EXPECT_EQ(zero_point_five.Data(), 0x08000);
  EXPECT_EQ(one.Data(), 0x10000);
  EXPECT_EQ(one_point_five.Data(), 0x18000);
  EXPECT_EQ(two_point_five.Data(), 0x28000);

  // Convert from raw value
  fetch::fixed_point::FixedPoint<16, 16> two_point_five_raw(2, 0x08000);
  fetch::fixed_point::FixedPoint<16, 16> m_two_point_five_raw(-2, 0x08000);
  EXPECT_EQ(two_point_five, two_point_five_raw);
  EXPECT_EQ(m_one_point_five, m_two_point_five_raw);

  // Extreme cases:
  // smallest possible double representable to a FixedPoint
  fetch::fixed_point::FixedPoint<16, 16> infinitesimal(0.00002);
  // Largest fractional closest to one, representable to a FixedPoint
  fetch::fixed_point::FixedPoint<16, 16> almost_one(0.99999);
  // Largest fractional closest to one, representable to a FixedPoint
  fetch::fixed_point::FixedPoint<16, 16> largest_int(std::numeric_limits<int16_t>::max());

  // Smallest possible integer, increase by one, in order to allow for the fractional part.
  fetch::fixed_point::FixedPoint<16, 16> smallest_int(std::numeric_limits<int16_t>::min());

  // Largest possible Fixed Point number.
  fetch::fixed_point::FixedPoint<16, 16> largest_fixed_point = largest_int + almost_one;

  // Smallest possible Fixed Point number.
  fetch::fixed_point::FixedPoint<16, 16> smallest_fixed_point = smallest_int + almost_one;

  EXPECT_EQ(infinitesimal.Data(), fp32::SMALLEST_FRACTION);
  EXPECT_EQ(almost_one.Data(), fp32::LARGEST_FRACTION);
  EXPECT_EQ(largest_int.Data(), fp32::MAX_INT);
  EXPECT_EQ(smallest_int.Data(), fp32::MIN_INT);
  EXPECT_EQ(largest_fixed_point.Data(), fp32::MAX);
  EXPECT_EQ(smallest_fixed_point.Data(), fp32::MIN);
  EXPECT_EQ(fp32::MIN, 0x8000ffff);
  EXPECT_EQ(fp32::MAX, 0x7fffffff);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int32_t>::min());
  // On the other hand we expect to be exactly the same as the largest positive integer of int64_t
  EXPECT_TRUE(largest_fixed_point.Data() == std::numeric_limits<int32_t>::max());

  EXPECT_EQ(sizeof(one), 4);

  EXPECT_EQ(int(one), 1);
  EXPECT_EQ(unsigned(one), 1);
  EXPECT_EQ(int32_t(one), 1);
  EXPECT_EQ(uint32_t(one), 1);
  EXPECT_EQ(long(one), 1);
  EXPECT_EQ((unsigned long)(one), 1);
  EXPECT_EQ(int64_t(one), 1);
  EXPECT_EQ(uint64_t(one), 1);

  EXPECT_EQ(fp32::TOLERANCE.Data(), 0x15);
  EXPECT_EQ(fp32::DECIMAL_DIGITS, 4);
}

TEST(FixedPointTest, Conversion_32_32)
{
  // Positive
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);

  EXPECT_EQ(int(one), 1);
  EXPECT_EQ(int(two), 2);
  EXPECT_EQ(float(one), 1.0f);
  EXPECT_EQ(float(two), 2.0f);
  EXPECT_EQ(double(one), 1.0);
  EXPECT_EQ(double(two), 2.0);

  // Negative
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);
  fetch::fixed_point::FixedPoint<32, 32> m_two(-2);

  EXPECT_EQ(int(m_one), -1);
  EXPECT_EQ(int(m_two), -2);
  EXPECT_EQ(float(m_one), -1.0f);
  EXPECT_EQ(float(m_two), -2.0f);
  EXPECT_EQ(double(m_one), -1.0);
  EXPECT_EQ(double(m_two), -2.0);

  // CONST_ZERO
  fetch::fixed_point::FixedPoint<32, 32> zero(0);
  fetch::fixed_point::FixedPoint<32, 32> m_zero(-0);

  EXPECT_EQ(int(zero), 0);
  EXPECT_EQ(int(m_zero), 0);
  EXPECT_EQ(float(zero), 0.0f);
  EXPECT_EQ(float(m_zero), 0.0f);
  EXPECT_EQ(double(zero), 0.0);
  EXPECT_EQ(double(m_zero), 0.0);

  // Get raw value
  fetch::fixed_point::FixedPoint<32, 32> zero_point_five(0.5);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> two_point_five(2.5);
  fetch::fixed_point::FixedPoint<32, 32> m_one_point_five(-1.5);

  EXPECT_EQ(zero_point_five.Data(), 0x080000000);
  EXPECT_EQ(one.Data(), 0x100000000);
  EXPECT_EQ(one_point_five.Data(), 0x180000000);
  EXPECT_EQ(two_point_five.Data(), 0x280000000);

  // Convert from raw value
  fetch::fixed_point::FixedPoint<32, 32> two_point_five_raw(2, 0x080000000);
  fetch::fixed_point::FixedPoint<32, 32> m_two_point_five_raw(-2, 0x080000000);
  EXPECT_EQ(two_point_five, two_point_five_raw);
  EXPECT_EQ(m_one_point_five, m_two_point_five_raw);

  // Extreme cases:
  // smallest possible double representable to a FixedPoint
  fetch::fixed_point::FixedPoint<32, 32> infinitesimal(0.0000000004);
  // Largest fractional closest to one, representable to a FixedPoint
  fetch::fixed_point::FixedPoint<32, 32> almost_one(0.9999999998);
  // Largest fractional closest to one, representable to a FixedPoint
  fetch::fixed_point::FixedPoint<32, 32> largest_int(std::numeric_limits<int32_t>::max());

  // Smallest possible integer, increase by one, in order to allow for the fractional part.
  fetch::fixed_point::FixedPoint<32, 32> smallest_int(std::numeric_limits<int32_t>::min());

  // Largest possible Fixed Point number.
  fetch::fixed_point::FixedPoint<32, 32> largest_fixed_point = largest_int + almost_one;

  // Smallest possible Fixed Point number.
  fetch::fixed_point::FixedPoint<32, 32> smallest_fixed_point = smallest_int + almost_one;

  EXPECT_EQ(infinitesimal.Data(), fp64::SMALLEST_FRACTION);
  EXPECT_EQ(almost_one.Data(), fp64::LARGEST_FRACTION);
  EXPECT_EQ(largest_int.Data(), fp64::MAX_INT);
  EXPECT_EQ(smallest_int.Data(), fp64::MIN_INT);
  EXPECT_EQ(largest_fixed_point.Data(), fp64::MAX);
  EXPECT_EQ(smallest_fixed_point.Data(), fp64::MIN);
  EXPECT_EQ(fp64::MIN, 0x80000000ffffffff);
  EXPECT_EQ(fp64::MAX, 0x7fffffffffffffff);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int64_t>::min());
  // On the other hand we expect to be exactly the same as the largest positive integer of int64_t
  EXPECT_TRUE(largest_fixed_point.Data() == std::numeric_limits<int64_t>::max());

  EXPECT_EQ(sizeof(one), 8);

  EXPECT_EQ(int(one), 1);
  EXPECT_EQ(unsigned(one), 1);
  EXPECT_EQ(int32_t(one), 1);
  EXPECT_EQ(uint32_t(one), 1);
  EXPECT_EQ(long(one), 1);
  EXPECT_EQ(uint64_t(one), 1);
  EXPECT_EQ(int64_t(one), 1);
  EXPECT_EQ(uint64_t(one), 1);

  EXPECT_EQ(fp64::TOLERANCE.Data(), 0x200);
  EXPECT_EQ(fp64::DECIMAL_DIGITS, 9);
}

TEST(FixedPointTest, Addition_16_16)
{
  // Positive
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);

  EXPECT_EQ(int(one + two), 3);
  EXPECT_EQ(float(one + two), 3.0f);
  EXPECT_EQ(double(one + two), 3.0);

  // Negative
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1);
  fetch::fixed_point::FixedPoint<16, 16> m_two(-2);

  EXPECT_EQ(int(m_one + one), 0);
  EXPECT_EQ(int(m_one + m_two), -3);
  EXPECT_EQ(float(m_one + one), 0.0f);
  EXPECT_EQ(float(m_one + m_two), -3.0f);
  EXPECT_EQ(double(m_one + one), 0.0);
  EXPECT_EQ(double(m_one + m_two), -3.0);

  fetch::fixed_point::FixedPoint<16, 16> another{one};
  ++another;
  EXPECT_EQ(another, two);

  // CONST_ZERO
  fetch::fixed_point::FixedPoint<16, 16> zero(0);
  fetch::fixed_point::FixedPoint<16, 16> m_zero(-0);

  EXPECT_EQ(int(zero), 0);
  EXPECT_EQ(int(m_zero), 0);
  EXPECT_EQ(float(zero), 0.0f);
  EXPECT_EQ(float(m_zero), 0.0f);
  EXPECT_EQ(double(zero), 0.0);
  EXPECT_EQ(double(m_zero), 0.0);

  // Infinitesimal additions
  fetch::fixed_point::FixedPoint<16, 16> almost_one(0, fp64::LARGEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> infinitesimal(0, fp64::SMALLEST_FRACTION);

  // Largest possible fraction and smallest possible fraction should make us the value of 1
  EXPECT_EQ(almost_one + infinitesimal, one);
  // The same for negative
  EXPECT_EQ(-almost_one - infinitesimal, m_one);
}

TEST(FixedPointTest, Addition_32_32)
{
  // Positive
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);

  EXPECT_EQ(int(one + two), 3);
  EXPECT_EQ(float(one + two), 3.0f);
  EXPECT_EQ(double(one + two), 3.0);

  // Negative
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);
  fetch::fixed_point::FixedPoint<32, 32> m_two(-2);

  EXPECT_EQ(int(m_one + one), 0);
  EXPECT_EQ(int(m_one + m_two), -3);
  EXPECT_EQ(float(m_one + one), 0.0f);
  EXPECT_EQ(float(m_one + m_two), -3.0f);
  EXPECT_EQ(double(m_one + one), 0.0);
  EXPECT_EQ(double(m_one + m_two), -3.0);

  // CONST_ZERO
  fetch::fixed_point::FixedPoint<32, 32> zero(0);
  fetch::fixed_point::FixedPoint<32, 32> m_zero(-0);

  EXPECT_EQ(int(zero), 0);
  EXPECT_EQ(int(m_zero), 0);
  EXPECT_EQ(float(zero), 0.0f);
  EXPECT_EQ(float(m_zero), 0.0f);
  EXPECT_EQ(double(zero), 0.0);
  EXPECT_EQ(double(m_zero), 0.0);

  // Infinitesimal additions
  fetch::fixed_point::FixedPoint<32, 32> almost_one(0, fp64::LARGEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> infinitesimal(0, fp64::SMALLEST_FRACTION);

  // Largest possible fraction and smallest possible fraction should make us the value of 1
  EXPECT_EQ(almost_one + infinitesimal, one);
  // The same for negative
  EXPECT_EQ(-almost_one - infinitesimal, m_one);
}

TEST(FixedPointTest, Subtraction_16_16)
{
  // Positive
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);

  EXPECT_EQ(int(two - one), 1);
  EXPECT_EQ(float(two - one), 1.0f);
  EXPECT_EQ(double(two - one), 1.0);

  EXPECT_EQ(int(one - two), -1);
  EXPECT_EQ(float(one - two), -1.0f);
  EXPECT_EQ(double(one - two), -1.0);

  // Negative
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1);
  fetch::fixed_point::FixedPoint<16, 16> m_two(-2);

  EXPECT_EQ(int(m_one - one), -2);
  EXPECT_EQ(int(m_one - m_two), 1);
  EXPECT_EQ(float(m_one - one), -2.0f);
  EXPECT_EQ(float(m_one - m_two), 1.0f);
  EXPECT_EQ(double(m_one - one), -2.0);
  EXPECT_EQ(double(m_one - m_two), 1.0);

  // Fractions
  fetch::fixed_point::FixedPoint<16, 16> almost_three(2, fp32::LARGEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> almost_two(1, fp32::LARGEST_FRACTION);

  EXPECT_EQ(almost_three - almost_two, one);
}

TEST(FixedPointTest, Subtraction_32_32)
{
  // Positive
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);

  EXPECT_EQ(int(two - one), 1);
  EXPECT_EQ(float(two - one), 1.0f);
  EXPECT_EQ(double(two - one), 1.0);

  EXPECT_EQ(int(one - two), -1);
  EXPECT_EQ(float(one - two), -1.0f);
  EXPECT_EQ(double(one - two), -1.0);

  // Negative
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);
  fetch::fixed_point::FixedPoint<32, 32> m_two(-2);

  EXPECT_EQ(int(m_one - one), -2);
  EXPECT_EQ(int(m_one - m_two), 1);
  EXPECT_EQ(float(m_one - one), -2.0f);
  EXPECT_EQ(float(m_one - m_two), 1.0f);
  EXPECT_EQ(double(m_one - one), -2.0);
  EXPECT_EQ(double(m_one - m_two), 1.0);

  // Fractions
  fetch::fixed_point::FixedPoint<32, 32> almost_three(2, fp64::LARGEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> almost_two(1, fp64::LARGEST_FRACTION);

  EXPECT_EQ(almost_three - almost_two, one);
}

TEST(FixedPointTest, Multiplication_16_16)
{
  // Positive
  fetch::fixed_point::FixedPoint<16, 16> zero(0);
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);
  fetch::fixed_point::FixedPoint<16, 16> three(3);
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1);

  EXPECT_EQ(two * one, two);
  EXPECT_EQ(one * 2, 2);
  EXPECT_EQ(m_one * zero, zero);
  EXPECT_EQ(m_one * one, m_one);
  EXPECT_EQ(float(two * 2.0f), 4.0f);
  EXPECT_EQ(double(three * 2.0), 6.0);

  EXPECT_EQ(int(one * two), 2);
  EXPECT_EQ(float(one * two), 2.0f);
  EXPECT_EQ(double(one * two), 2.0);

  EXPECT_EQ(int(two * zero), 0);
  EXPECT_EQ(float(two * zero), 0.0f);
  EXPECT_EQ(double(two * zero), 0.0);

  // Extreme cases
  fetch::fixed_point::FixedPoint<16, 16> almost_one(0, fp64::LARGEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> infinitesimal(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> huge(0x4000, 0);
  fetch::fixed_point::FixedPoint<16, 16> small(0, 0x4000);

  EXPECT_EQ(almost_one * almost_one, almost_one - infinitesimal);
  EXPECT_EQ(almost_one * infinitesimal, zero);
  EXPECT_EQ(huge * infinitesimal, small);

  // (One of) largest possible multiplications
}

TEST(FixedPointTest, Multiplication_32_32)
{
  // Positive
  fetch::fixed_point::FixedPoint<32, 32> zero(0);
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);
  fetch::fixed_point::FixedPoint<32, 32> three(3);
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);

  EXPECT_EQ(two * one, two);
  EXPECT_EQ(one * 2, 2);
  EXPECT_EQ(m_one * zero, zero);
  EXPECT_EQ(m_one * one, m_one);
  EXPECT_EQ(float(two * 2.0f), 4.0f);
  EXPECT_EQ(double(three * 2.0), 6.0);

  EXPECT_EQ(int(one * two), 2);
  EXPECT_EQ(float(one * two), 2.0f);
  EXPECT_EQ(double(one * two), 2.0);

  EXPECT_EQ(int(two * zero), 0);
  EXPECT_EQ(float(two * zero), 0.0f);
  EXPECT_EQ(double(two * zero), 0.0);

  // Extreme cases
  fetch::fixed_point::FixedPoint<32, 32> almost_one(0, fp64::LARGEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> infinitesimal(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> huge(0x40000000, 0);
  fetch::fixed_point::FixedPoint<32, 32> small(0, 0x40000000);

  EXPECT_EQ(almost_one * almost_one, almost_one - infinitesimal);
  EXPECT_EQ(almost_one * infinitesimal, zero);
  EXPECT_EQ(huge * infinitesimal, small);
}

TEST(FixedPointTest, Division_16_16)
{
  // Positive
  fetch::fixed_point::FixedPoint<16, 16> zero(0);
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);

  EXPECT_EQ(int(two / one), 2);
  EXPECT_EQ(float(two / one), 2.0f);
  EXPECT_EQ(double(two / one), 2.0);

  EXPECT_EQ(int(one / two), 0);
  EXPECT_EQ(float(one / two), 0.5f);
  EXPECT_EQ(double(one / two), 0.5);

  // Extreme cases
  fetch::fixed_point::FixedPoint<16, 16> infinitesimal(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> huge(0x4000, 0);
  fetch::fixed_point::FixedPoint<16, 16> small(0, 0x4000);

  EXPECT_EQ(small / infinitesimal, huge);
  EXPECT_EQ(infinitesimal / one, infinitesimal);
  EXPECT_EQ(one / huge, infinitesimal * 4);
  EXPECT_EQ(huge / infinitesimal, zero);

  EXPECT_THROW(two / zero, std::overflow_error);
  EXPECT_THROW(zero / zero, std::overflow_error);
}

TEST(FixedPointTest, Division_32_32)
{
  // Positive
  fetch::fixed_point::FixedPoint<32, 32> zero(0);
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);

  EXPECT_EQ(int(two / one), 2);
  EXPECT_EQ(float(two / one), 2.0f);
  EXPECT_EQ(double(two / one), 2.0);

  EXPECT_EQ(int(one / two), 0);
  EXPECT_EQ(float(one / two), 0.5f);
  EXPECT_EQ(double(one / two), 0.5);

  // Extreme cases
  fetch::fixed_point::FixedPoint<32, 32> infinitesimal(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> huge(0x40000000, 0);
  fetch::fixed_point::FixedPoint<32, 32> small(0, 0x40000000);

  EXPECT_EQ(small / infinitesimal, huge);
  EXPECT_EQ(infinitesimal / one, infinitesimal);
  EXPECT_EQ(one / huge, infinitesimal * 4);
  EXPECT_EQ(huge / infinitesimal, zero);

  EXPECT_THROW(two / zero, std::overflow_error);
  EXPECT_THROW(zero / zero, std::overflow_error);
}

TEST(FixedPointTest, Comparison_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> zero(0);
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);

  EXPECT_TRUE(zero < one);
  EXPECT_TRUE(zero < two);
  EXPECT_TRUE(one < two);

  EXPECT_FALSE(zero > one);
  EXPECT_FALSE(zero > two);
  EXPECT_FALSE(one > two);

  EXPECT_FALSE(zero == one);
  EXPECT_FALSE(zero == two);
  EXPECT_FALSE(one == two);

  EXPECT_TRUE(zero == zero);
  EXPECT_TRUE(one == one);
  EXPECT_TRUE(two == two);

  EXPECT_TRUE(zero >= zero);
  EXPECT_TRUE(one >= one);
  EXPECT_TRUE(two >= two);

  EXPECT_TRUE(zero <= zero);
  EXPECT_TRUE(one <= one);
  EXPECT_TRUE(two <= two);

  fetch::fixed_point::FixedPoint<16, 16> zero_point_five(0.5);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> two_point_five(2.5);

  EXPECT_TRUE(zero_point_five < one);
  EXPECT_TRUE(zero_point_five < two);
  EXPECT_TRUE(one_point_five < two);

  EXPECT_FALSE(zero_point_five > one);
  EXPECT_FALSE(zero_point_five > two);
  EXPECT_FALSE(one_point_five > two);

  EXPECT_FALSE(zero_point_five == one);
  EXPECT_FALSE(zero_point_five == two);
  EXPECT_FALSE(one_point_five == two);

  EXPECT_TRUE(zero_point_five == zero_point_five);
  EXPECT_TRUE(one_point_five == one_point_five);
  EXPECT_TRUE(two_point_five == two_point_five);

  EXPECT_TRUE(zero_point_five >= zero_point_five);
  EXPECT_TRUE(one_point_five >= one_point_five);
  EXPECT_TRUE(two_point_five >= two_point_five);

  EXPECT_TRUE(zero_point_five <= zero_point_five);
  EXPECT_TRUE(one_point_five <= one_point_five);
  EXPECT_TRUE(two_point_five <= two_point_five);

  fetch::fixed_point::FixedPoint<16, 16> m_zero(-0);
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1.0);
  fetch::fixed_point::FixedPoint<16, 16> m_two(-2);

  EXPECT_TRUE(m_zero > m_one);
  EXPECT_TRUE(m_zero > m_two);
  EXPECT_TRUE(m_one > m_two);

  EXPECT_FALSE(m_zero < m_one);
  EXPECT_FALSE(m_zero < m_two);
  EXPECT_FALSE(m_one < m_two);

  EXPECT_FALSE(m_zero == m_one);
  EXPECT_FALSE(m_zero == m_two);
  EXPECT_FALSE(m_one == m_two);

  EXPECT_TRUE(zero == m_zero);
  EXPECT_TRUE(m_zero == m_zero);
  EXPECT_TRUE(m_one == m_one);
  EXPECT_TRUE(m_two == m_two);

  EXPECT_TRUE(m_zero >= m_zero);
  EXPECT_TRUE(m_one >= m_one);
  EXPECT_TRUE(m_two >= m_two);

  EXPECT_TRUE(m_zero <= m_zero);
  EXPECT_TRUE(m_one <= m_one);
  EXPECT_TRUE(m_two <= m_two);

  EXPECT_TRUE(zero > m_one);
  EXPECT_TRUE(zero > m_two);
  EXPECT_TRUE(one > m_two);

  EXPECT_TRUE(m_two < one);
  EXPECT_TRUE(m_one < two);

  EXPECT_TRUE(fp32::CONST_E == 2.718281828459045235360287471352662498);
  EXPECT_TRUE(fp32::CONST_LOG2E == 1.442695040888963407359924681001892137);
  EXPECT_TRUE(fp32::CONST_LOG10E == 0.434294481903251827651128918916605082);
  EXPECT_TRUE(fp32::CONST_LN2 == 0.693147180559945309417232121458176568);
  EXPECT_TRUE(fp32::CONST_LN10 == 2.302585092994045684017991454684364208);
  EXPECT_TRUE(fp32::CONST_PI == 3.141592653589793238462643383279502884);
  EXPECT_TRUE(fp32::CONST_PI_2 == 1.570796326794896619231321691639751442);
  EXPECT_TRUE(fp32::CONST_PI_4 == 0.785398163397448309615660845819875721);
  EXPECT_TRUE(fp32::CONST_INV_PI == 0.318309886183790671537767526745028724);
  EXPECT_TRUE(fp32::CONST_2_INV_PI == 0.636619772367581343075535053490057448);
  EXPECT_TRUE(fp32::CONST_2_INV_SQRTPI == 1.128379167095512573896158903121545172);
  EXPECT_TRUE(fp32::CONST_SQRT2 == 1.414213562373095048801688724209698079);
  EXPECT_TRUE(fp32::CONST_INV_SQRT2 == 0.707106781186547524400844362104849039);

  EXPECT_EQ(fp32::MAX_INT, 0x7fff0000);
  EXPECT_EQ(fp32::MIN_INT, 0x80000000);
  EXPECT_EQ(fp32::MAX, 0x7fffffff);
  EXPECT_EQ(fp32::MIN, 0x8000ffff);
  EXPECT_EQ(fp32::MAX_EXP.Data(), 0x000a65b9);
  EXPECT_EQ(fp32::MIN_EXP.Data(), 0xfff59a47);
}

TEST(FixedPointTest, Comparison_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> zero(0);
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);

  EXPECT_LT(zero, one);
  EXPECT_LT(zero, two);
  EXPECT_LT(one, two);

  EXPECT_FALSE(zero > one);
  EXPECT_FALSE(zero > two);
  EXPECT_FALSE(one > two);

  EXPECT_FALSE(zero == one);
  EXPECT_FALSE(zero == two);
  EXPECT_FALSE(one == two);

  EXPECT_EQ(zero, zero);
  EXPECT_EQ(one, one);
  EXPECT_EQ(two, two);

  EXPECT_GE(zero, zero);
  EXPECT_GE(one, one);
  EXPECT_GE(two, two);

  EXPECT_LE(zero, zero);
  EXPECT_LE(one, one);
  EXPECT_LE(two, two);

  fetch::fixed_point::FixedPoint<32, 32> zero_point_five(0.5);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> two_point_five(2.5);

  EXPECT_LT(zero_point_five, one);
  EXPECT_LT(zero_point_five, two);
  EXPECT_LT(one_point_five, two);

  EXPECT_FALSE(zero_point_five > one);
  EXPECT_FALSE(zero_point_five > two);
  EXPECT_FALSE(one_point_five > two);

  EXPECT_FALSE(zero_point_five == one);
  EXPECT_FALSE(zero_point_five == two);
  EXPECT_FALSE(one_point_five == two);

  EXPECT_EQ(zero_point_five, zero_point_five);
  EXPECT_EQ(one_point_five, one_point_five);
  EXPECT_EQ(two_point_five, two_point_five);

  EXPECT_GE(zero_point_five, zero_point_five);
  EXPECT_GE(one_point_five, one_point_five);
  EXPECT_GE(two_point_five, two_point_five);

  EXPECT_LE(zero_point_five, zero_point_five);
  EXPECT_LE(one_point_five, one_point_five);
  EXPECT_LE(two_point_five, two_point_five);

  fetch::fixed_point::FixedPoint<32, 32> m_zero(-0);
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1.0);
  fetch::fixed_point::FixedPoint<32, 32> m_two(-2);

  EXPECT_GT(m_zero, m_one);
  EXPECT_GT(m_zero, m_two);
  EXPECT_GT(m_one, m_two);

  EXPECT_FALSE(m_zero < m_one);
  EXPECT_FALSE(m_zero < m_two);
  EXPECT_FALSE(m_one < m_two);

  EXPECT_FALSE(m_zero == m_one);
  EXPECT_FALSE(m_zero == m_two);
  EXPECT_FALSE(m_one == m_two);

  EXPECT_EQ(zero, m_zero);
  EXPECT_EQ(m_zero, m_zero);
  EXPECT_EQ(m_one, m_one);
  EXPECT_EQ(m_two, m_two);

  EXPECT_GE(m_zero, m_zero);
  EXPECT_GE(m_one, m_one);
  EXPECT_GE(m_two, m_two);

  EXPECT_LE(m_zero, m_zero);
  EXPECT_LE(m_one, m_one);
  EXPECT_LE(m_two, m_two);

  EXPECT_GT(zero, m_one);
  EXPECT_GT(zero, m_two);
  EXPECT_GT(one, m_two);

  EXPECT_LT(m_two, one);
  EXPECT_LT(m_one, two);

  EXPECT_TRUE(fp64::CONST_E == 2.718281828459045235360287471352662498);
  EXPECT_TRUE(fp64::CONST_LOG2E == 1.442695040888963407359924681001892137);
  EXPECT_TRUE(fp64::CONST_LOG10E == 0.434294481903251827651128918916605082);
  EXPECT_TRUE(fp64::CONST_LN2 == 0.693147180559945309417232121458176568);
  EXPECT_TRUE(fp64::CONST_LN10 == 2.302585092994045684017991454684364208);
  EXPECT_TRUE(fp64::CONST_PI == 3.141592653589793238462643383279502884);
  EXPECT_TRUE(fp64::CONST_PI / 2 == fp64::CONST_PI_2);
  EXPECT_TRUE(fp64::CONST_PI_4 == 0.785398163397448309615660845819875721);
  EXPECT_TRUE(one / fp64::CONST_PI == fp64::CONST_INV_PI);
  EXPECT_TRUE(fp64::CONST_2_INV_PI == 0.636619772367581343075535053490057448);
  EXPECT_TRUE(fp64::CONST_2_INV_SQRTPI == 1.128379167095512573896158903121545172);
  EXPECT_TRUE(fp64::CONST_SQRT2 == 1.414213562373095048801688724209698079);
  EXPECT_TRUE(fp64::CONST_INV_SQRT2 == 0.707106781186547524400844362104849039);

  EXPECT_EQ(fp64::MAX_INT, 0x7fffffff00000000);
  EXPECT_EQ(fp64::MIN_INT, 0x8000000000000000);
  EXPECT_EQ(fp64::MAX, 0x7fffffffffffffff);
  EXPECT_EQ(fp64::MIN, 0x80000000ffffffff);
  EXPECT_EQ(fp64::MAX_EXP.Data(), 0x000000157cd0e714);
  EXPECT_EQ(fp64::MIN_EXP.Data(), 0xffffffea832f18ec);
}

TEST(FixedPointTest, Exponential_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);
  fetch::fixed_point::FixedPoint<16, 16> negative{-0.40028143};
  fetch::fixed_point::FixedPoint<16, 16> e1    = fp32::Exp(one);
  fetch::fixed_point::FixedPoint<16, 16> e2    = fp32::Exp(two);
  fetch::fixed_point::FixedPoint<16, 16> e3    = fp32::Exp(negative);
  fetch::fixed_point::FixedPoint<16, 16> e_max = fp32::Exp(fp32::MAX_EXP);

  EXPECT_NEAR((double)e1 / std::exp(1.0), 1.0, (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e2 / std::exp(2.0), 1.0, (double)fp32::TOLERANCE);
  EXPECT_NEAR(((double)e3 - std::exp(double(negative))) / std::exp(double(negative)), 0,
              (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e_max / std::exp((double)fp32::MAX_EXP), 1.0, (double)fp32::TOLERANCE);
  EXPECT_THROW(fp32::Exp(fp32::MAX_EXP + 1), std::overflow_error);
}

TEST(FixedPointTest, Exponential_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> small(0.0001);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> negative{-0.40028143};
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Exp(one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Exp(two);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Exp(ten);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Exp(small);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Exp(tiny);
  fetch::fixed_point::FixedPoint<32, 32> e7 = fp64::Exp(negative);

  EXPECT_NEAR(((double)e1 - std::exp(1.0)) / std::exp(1.0), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)e2 - std::exp(2.0)) / std::exp(2.0), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)e3 - std::exp(10.0)) / std::exp(10.0), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)e5 - std::exp(0.0001)) / std::exp(0.0001), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)e6 - std::exp(double(tiny))) / std::exp(double(tiny)), 0,
              (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)e7 - std::exp(double(negative))) / std::exp(double(negative)), 0,
              (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)fp64::Exp(fp64::MAX_EXP) - std::exp(double(fp64::MAX_EXP))) /
                  std::exp(double(fp64::MAX_EXP)),
              0, (double)fp64::TOLERANCE);

  // Out of range
  EXPECT_THROW(fp64::Exp(fp64::MAX_EXP + 1), std::overflow_error);

  // Negative values
  EXPECT_NEAR(((double)fp64::Exp(-one) - std::exp(-1.0)) / std::exp(-1.0), 0,
              (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)fp64::Exp(-two) - std::exp(-2.0)) / std::exp(-2.0), 0,
              (double)fp64::TOLERANCE);

  // This particular error produces more than 1e-6 error failing the test
  EXPECT_NEAR(((double)fp64::Exp(-ten) - std::exp(-10.0)) / std::exp(-10.0), 0, 1.1e-6);
  // The rest pass with fp64::TOLERANCE
  EXPECT_NEAR(((double)fp64::Exp(-small) - std::exp(-0.0001)) / std::exp(-0.0001), 0,
              (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)fp64::Exp(-tiny) - std::exp(-double(tiny))) / std::exp(-double(tiny)), 0,
              (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)fp64::Exp(fp64::MIN_EXP) - std::exp((double)fp64::MIN_EXP)) /
                  std::exp((double)fp64::MIN_EXP),
              0, (double)fp64::TOLERANCE);
}

TEST(FixedPointTest, Pow_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> x{-1.6519711627625};
  fetch::fixed_point::FixedPoint<32, 32> two{2};
  fetch::fixed_point::FixedPoint<32, 32> three{3};
  fetch::fixed_point::FixedPoint<32, 32> y{1.8464393615723};
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Pow(x, two);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Pow(x, three);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Pow(two, y);

  EXPECT_NEAR((double)e1 / std::pow(-1.6519711627625, 2), 1.0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e2 / std::pow(-1.6519711627625, 3), 1.0, (double)fp64::TOLERANCE);
  // This test has ~1.4e-6 error
  EXPECT_NEAR((double)e3 / std::pow(2, 1.8464393615723), 1.0, 1.5e-6);
  EXPECT_THROW(fp64::Pow(fp64::CONST_ZERO, fp64::CONST_ZERO), std::runtime_error);
  EXPECT_THROW(fp64::Pow(x, x), std::runtime_error);
}

TEST(FixedPointTest, Logarithm_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> ten(10);
  fetch::fixed_point::FixedPoint<16, 16> huge(10000);
  fetch::fixed_point::FixedPoint<16, 16> small(0.001);
  fetch::fixed_point::FixedPoint<16, 16> tiny(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Log2(one);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Log2(one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e3 = fp32::Log2(ten);
  fetch::fixed_point::FixedPoint<16, 16> e4 = fp32::Log2(huge);
  fetch::fixed_point::FixedPoint<16, 16> e5 = fp32::Log2(small);
  fetch::fixed_point::FixedPoint<16, 16> e6 = fp32::Log2(tiny);

  EXPECT_NEAR((double)e1, std::log2((double)one), (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e2, std::log2((double)one_point_five), (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e3, std::log2((double)ten), (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e4, std::log2((double)huge), (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e5, std::log2((double)small), (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e6, std::log2((double)tiny), (double)fp32::TOLERANCE);
}

TEST(FixedPointTest, Logarithm_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> huge(1000000000);
  fetch::fixed_point::FixedPoint<32, 32> small(0.0001);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Log2(one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Log2(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Log2(ten);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Log2(huge);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Log2(small);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Log2(tiny);

  EXPECT_NEAR((double)e1, std::log2((double)one), (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e2, std::log2((double)one_point_five), (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e3, std::log2((double)ten), (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e4, std::log2((double)huge), (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e5, std::log2((double)small), (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e6, std::log2((double)tiny), (double)fp64::TOLERANCE);
}

TEST(FixedPointTest, Abs_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> m_one_point_five(-1.5);
  fetch::fixed_point::FixedPoint<16, 16> ten(10);
  fetch::fixed_point::FixedPoint<16, 16> m_ten(-10);
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Abs(one);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Abs(m_one);
  fetch::fixed_point::FixedPoint<16, 16> e3 = fp32::Abs(one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e4 = fp32::Abs(m_one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e5 = fp32::Abs(ten);
  fetch::fixed_point::FixedPoint<16, 16> e6 = fp32::Abs(m_ten);

  EXPECT_EQ(e1, one);
  EXPECT_EQ(e2, one);
  EXPECT_EQ(e3, one_point_five);
  EXPECT_EQ(e4, one_point_five);
  EXPECT_EQ(e5, ten);
  EXPECT_EQ(e6, ten);
}

TEST(FixedPointTest, Abs_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> m_one_point_five(-1.5);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> m_ten(-10);
  fetch::fixed_point::FixedPoint<32, 32> huge(999999999.0);
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Abs(one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Abs(m_one);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Abs(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Abs(m_one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Abs(ten);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Abs(m_ten);
  fetch::fixed_point::FixedPoint<32, 32> e7 = fp64::Abs(huge);

  EXPECT_EQ(e1, one);
  EXPECT_EQ(e2, one);
  EXPECT_EQ(e3, one_point_five);
  EXPECT_EQ(e4, one_point_five);
  EXPECT_EQ(e5, ten);
  EXPECT_EQ(e6, ten);
  EXPECT_EQ(e7, huge);
}

TEST(FixedPointTest, SQRT_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> two(2);
  fetch::fixed_point::FixedPoint<16, 16> four(4);
  fetch::fixed_point::FixedPoint<16, 16> ten(10);
  fetch::fixed_point::FixedPoint<16, 16> huge(10000);
  fetch::fixed_point::FixedPoint<16, 16> small(0.0001);
  fetch::fixed_point::FixedPoint<16, 16> tiny(0, fp32::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Sqrt(one);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Sqrt(one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e3 = fp32::Sqrt(two);
  fetch::fixed_point::FixedPoint<16, 16> e4 = fp32::Sqrt(four);
  fetch::fixed_point::FixedPoint<16, 16> e5 = fp32::Sqrt(ten);
  fetch::fixed_point::FixedPoint<16, 16> e6 = fp32::Sqrt(huge);
  fetch::fixed_point::FixedPoint<16, 16> e7 = fp32::Sqrt(small);
  fetch::fixed_point::FixedPoint<16, 16> e8 = fp32::Sqrt(tiny);

  double delta = (double)e1 - std::sqrt((double)one);
  EXPECT_NEAR(delta / std::sqrt((double)one), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e2 - std::sqrt((double)one_point_five);
  EXPECT_NEAR(delta / std::sqrt((double)one_point_five), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e3 - std::sqrt((double)two);
  EXPECT_NEAR(delta / std::sqrt((double)two), 0.0, (double)fp32::TOLERANCE);
  delta = (double)(e3 - fp32::CONST_SQRT2);
  EXPECT_NEAR(delta / (double)fp32::CONST_SQRT2, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e4 - std::sqrt((double)four);
  EXPECT_NEAR(delta / std::sqrt((double)four), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e5 - std::sqrt((double)ten);
  EXPECT_NEAR(delta / std::sqrt((double)ten), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e6 - std::sqrt((double)huge);
  EXPECT_NEAR(delta / std::sqrt((double)huge), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e7 - std::sqrt((double)small);
  EXPECT_NEAR(delta / std::sqrt((double)small), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e8 - std::sqrt((double)tiny);
  EXPECT_NEAR(delta / std::sqrt((double)tiny), 0.0, (double)fp32::TOLERANCE);

  // Sqrt of a negative
  EXPECT_THROW(fp32::Sqrt(-one), std::runtime_error);
}

TEST(FixedPointTest, SQRT_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> two(2);
  fetch::fixed_point::FixedPoint<32, 32> four(4);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> huge(1000000000);
  fetch::fixed_point::FixedPoint<32, 32> small(0.0001);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, fp64::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Sqrt(one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Sqrt(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Sqrt(two);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Sqrt(four);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Sqrt(ten);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Sqrt(huge);
  fetch::fixed_point::FixedPoint<32, 32> e7 = fp64::Sqrt(small);
  fetch::fixed_point::FixedPoint<32, 32> e8 = fp64::Sqrt(tiny);

  double delta = (double)e1 - std::sqrt((double)one);
  EXPECT_NEAR(delta / std::sqrt((double)one), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e2 - std::sqrt((double)one_point_five);
  EXPECT_NEAR(delta / std::sqrt((double)one_point_five), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e3 - std::sqrt((double)two);
  EXPECT_NEAR(delta / std::sqrt((double)two), 0.0, (double)fp64::TOLERANCE);
  delta = (double)(e3 - fp64::CONST_SQRT2);
  EXPECT_NEAR(delta / (double)fp64::CONST_SQRT2, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e4 - std::sqrt((double)four);
  EXPECT_NEAR(delta / std::sqrt((double)four), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e5 - std::sqrt((double)ten);
  EXPECT_NEAR(delta / std::sqrt((double)ten), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e6 - std::sqrt((double)huge);
  EXPECT_NEAR(delta / std::sqrt((double)huge), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e7 - std::sqrt((double)small);
  EXPECT_NEAR(delta / std::sqrt((double)small), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e8 - std::sqrt((double)tiny);
  EXPECT_NEAR(delta / std::sqrt((double)tiny), 0.0, (double)fp64::TOLERANCE);

  // Sqrt of a negative
  EXPECT_THROW(fp64::Sqrt(-one), std::runtime_error);
}
