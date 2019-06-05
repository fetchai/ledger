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

#include <gtest/gtest.h>

#include <array>
#include <cmath>
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

  // _0
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

  // _0
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

  // _0
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

  // _0
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

  fp32   step{0.001};
  fp32   x{-10.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp32::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp32   e     = fp32::Exp(x);
    double r     = std::exp((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, 10 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Exp: max error = " << max_error << std::endl;
  std::cout << "Exp: avg error = " << avg_error << std::endl;
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
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Exp(small);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Exp(tiny);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Exp(negative);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Exp(ten);

  EXPECT_NEAR((double)e1 - std::exp((double)one), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e2 - std::exp((double)two), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e3 - std::exp((double)small), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e4 - std::exp(double(tiny)), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e5 - std::exp(double(negative)), 0, (double)fp64::TOLERANCE);

  // For bigger values check relative error
  EXPECT_NEAR(((double)e6 - std::exp((double)ten)) / std::exp((double)ten), 0,
              (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)fp64::Exp(fp64::MAX_EXP) - std::exp(double(fp64::MAX_EXP))) /
                  std::exp(double(fp64::MAX_EXP)),
              0, (double)fp64::TOLERANCE);

  // Out of range
  EXPECT_THROW(fp64::Exp(fp64::MAX_EXP + 1), std::overflow_error);

  // Negative values
  EXPECT_NEAR(((double)fp64::Exp(-one) - std::exp(-(double)one)), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR(((double)fp64::Exp(-two) - std::exp(-(double)two)), 0, (double)fp64::TOLERANCE);

  // This particular error produces more than 1e-6 error failing the test
  EXPECT_NEAR((double)fp64::Exp(-ten) - std::exp(-(double)ten), 0, (double)fp64::TOLERANCE);
  // The rest pass with fp64::TOLERANCE
  EXPECT_NEAR((double)fp64::Exp(-small) - std::exp(-(double)small), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)fp64::Exp(-tiny) - std::exp(-double(tiny)), 0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)fp64::Exp(fp64::MIN_EXP) - std::exp((double)fp64::MIN_EXP), 0,
              (double)fp64::TOLERANCE);

  fp64   step{0.0001};
  fp64   x{-10.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::Exp(x);
    double r     = std::exp((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, 10 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Exp: max error = " << max_error << std::endl;
  std::cout << "Exp: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_16_16_positive_x)
{
  fetch::fixed_point::FixedPoint<16, 16> a{-1.6519711627625};
  fetch::fixed_point::FixedPoint<16, 16> two{2};
  fetch::fixed_point::FixedPoint<16, 16> three{3};
  fetch::fixed_point::FixedPoint<16, 16> b{1.8464393615723};
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Pow(a, two);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Pow(a, three);
  fetch::fixed_point::FixedPoint<16, 16> e3 = fp32::Pow(two, b);

  EXPECT_NEAR((double)e1 / std::pow(-1.6519711627625, 2), 1.0, (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e2 / std::pow(-1.6519711627625, 3), 1.0, (double)fp32::TOLERANCE);
  EXPECT_NEAR((double)e3 / std::pow(2, 1.8464393615723), 1.0, (double)fp32::TOLERANCE);
  EXPECT_TRUE(fp32::isNaN(fp32::Pow(fp32::_0, fp32::_0)));
  EXPECT_TRUE(fp32::isNaN(fp32::Pow(a, a)));

  fp32   step{0.001};
  fp32   x{0.001};
  fp32   y{0.001};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 100.0; x += step)
  {
    for (; y < 10.5; y += step)
    {
      fp32   e     = fp32::Pow(x, y);
      double r     = std::pow((double)x, (double)y);
      double delta = std::abs((double)e - r);
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Pow: max error = " << max_error << std::endl;
  std::cout << "Pow: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_16_16_negative_x)
{
  fp32   step{0.01};
  fp32   x{-10};
  fp32   y{-4};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 10.0; x += step)
  {
    for (; y < 4; ++y)
    {
      fp32   e     = fp32::Pow(x, y);
      double r     = std::pow((double)x, (double)y);
      double delta = std::abs((double)e - r);
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Pow: max error = " << max_error << std::endl;
  std::cout << "Pow: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_32_32_positive_x)
{
  fetch::fixed_point::FixedPoint<32, 32> a{-1.6519711627625};
  fetch::fixed_point::FixedPoint<32, 32> two{2};
  fetch::fixed_point::FixedPoint<32, 32> three{3};
  fetch::fixed_point::FixedPoint<32, 32> b{1.8464393615723};
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Pow(a, two);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Pow(a, three);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Pow(two, b);

  EXPECT_NEAR((double)e1 / std::pow(-1.6519711627625, 2), 1.0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e2 / std::pow(-1.6519711627625, 3), 1.0, (double)fp64::TOLERANCE);
  EXPECT_NEAR((double)e3 / std::pow(2, 1.8464393615723), 1.0, (double)fp64::TOLERANCE);
  EXPECT_TRUE(fp64::isNaN(fp64::Pow(fp64::_0, fp64::_0)));
  EXPECT_TRUE(fp64::isNaN(fp64::Pow(a, a)));

  fp64   step{0.0001};
  fp64   x{0.0001};
  fp64   y{0.001};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 100.0; x += step)
  {
    for (; y < 40.5; y += step)
    {
      fp64   e     = fp64::Pow(x, y);
      double r     = std::pow((double)x, (double)y);
      double delta = std::abs((double)e - r);
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Pow: max error = " << max_error << std::endl;
  std::cout << "Pow: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_32_32_negative_x)
{
  fp64   step{0.01};
  fp64   x{-10};
  fp64   y{-9};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 10.0; x += step)
  {
    for (; y < 9; ++y)
    {
      fp64   e     = fp64::Pow(x, y);
      double r     = std::pow((double)x, (double)y);
      double delta = std::abs((double)e - r);
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Pow: max error = " << max_error << std::endl;
  std::cout << "Pow: avg error = " << avg_error << std::endl;
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

  fp32   step{0.001};
  fp32   x{tiny};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp32   e     = fp32::Log(x);
    double r     = std::log((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Log: max error = " << max_error << std::endl;
  std::cout << "Log: avg error = " << avg_error << std::endl;
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

  fp64   step{0.0001};
  fp64   x{tiny};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::Log(x);
    double r     = std::log((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
    if (delta > tolerance)
    {
      std::cout << "delta = " << delta << std::endl;
      std::cout << "e = " << e << std::endl;
      std::cout << "r = " << r << std::endl;
      std::cout << "x = " << x << std::endl;
    }
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Log: max error = " << max_error << std::endl;
  std::cout << "Log: avg error = " << avg_error << std::endl;
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

TEST(FixedPointTest, Remainder_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> m_one_point_five(-1.5);
  fetch::fixed_point::FixedPoint<16, 16> ten(10);
  fetch::fixed_point::FixedPoint<16, 16> m_ten(-10);
  fetch::fixed_point::FixedPoint<16, 16> x{1.6519711627625};
  fetch::fixed_point::FixedPoint<16, 16> huge(1000);
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Remainder(ten, one);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Remainder(ten, m_one);
  fetch::fixed_point::FixedPoint<16, 16> e3 = fp32::Remainder(ten, one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e4 = fp32::Remainder(ten, m_one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e5 = fp32::Remainder(ten, x);
  fetch::fixed_point::FixedPoint<16, 16> e6 = fp32::Remainder(m_ten, x);
  fetch::fixed_point::FixedPoint<16, 16> e7 = fp32::Remainder(huge, fp32::CONST_PI);

  EXPECT_EQ(e1, std::remainder((double)ten, (double)one));
  EXPECT_EQ(e2, std::remainder((double)ten, (double)m_one));
  EXPECT_EQ(e3, std::remainder((double)ten, (double)one_point_five));
  EXPECT_EQ(e4, std::remainder((double)ten, (double)m_one_point_five));
  EXPECT_EQ(e5, std::remainder((double)ten, (double)x));
  EXPECT_EQ(e6, std::remainder((double)m_ten, (double)x));
  EXPECT_EQ(e7, std::remainder((double)huge, (double)fp32::CONST_PI));
}

TEST(FixedPointTest, Remainder_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> m_one_point_five(-1.5);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> m_ten(-10);
  fetch::fixed_point::FixedPoint<32, 32> x{1.6519711627625};
  fetch::fixed_point::FixedPoint<32, 32> huge(1000000000);
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Remainder(ten, one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Remainder(ten, m_one);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Remainder(ten, one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Remainder(ten, m_one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Remainder(ten, x);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Remainder(m_ten, x);
  fetch::fixed_point::FixedPoint<32, 32> e7 = fp64::Remainder(huge, x);

  EXPECT_EQ(e1, std::remainder((double)ten, (double)one));
  EXPECT_EQ(e2, std::remainder((double)ten, (double)m_one));
  EXPECT_EQ(e3, std::remainder((double)ten, (double)one_point_five));
  EXPECT_EQ(e4, std::remainder((double)ten, (double)m_one_point_five));
  EXPECT_EQ(e5, std::remainder((double)ten, (double)x));
  EXPECT_EQ(e6, std::remainder((double)m_ten, (double)x));
  EXPECT_EQ(e7, std::remainder((double)huge, (double)x));
}

TEST(FixedPointTest, Fmod_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> m_one(-1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> m_one_point_five(-1.5);
  fetch::fixed_point::FixedPoint<16, 16> ten(10);
  fetch::fixed_point::FixedPoint<16, 16> m_ten(-10);
  fetch::fixed_point::FixedPoint<16, 16> x{1.6519711627625};
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Fmod(ten, one);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Fmod(ten, m_one);
  fetch::fixed_point::FixedPoint<16, 16> e3 = fp32::Fmod(ten, one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e4 = fp32::Fmod(ten, m_one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e5 = fp32::Fmod(ten, x);
  fetch::fixed_point::FixedPoint<16, 16> e6 = fp32::Fmod(m_ten, x);

  EXPECT_EQ(e1, std::fmod((double)ten, (double)one));
  EXPECT_EQ(e2, std::fmod((double)ten, (double)m_one));
  EXPECT_EQ(e3, std::fmod((double)ten, (double)one_point_five));
  EXPECT_EQ(e4, std::fmod((double)ten, (double)m_one_point_five));
  EXPECT_EQ(e5, std::fmod((double)ten, (double)x));
  EXPECT_EQ(e6, std::fmod((double)m_ten, (double)x));
}

TEST(FixedPointTest, Fmod_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> m_one_point_five(-1.5);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> m_ten(-10);
  fetch::fixed_point::FixedPoint<32, 32> x{1.6519711627625};
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Fmod(ten, one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Fmod(ten, m_one);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Fmod(ten, one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Fmod(ten, m_one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Fmod(ten, x);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Fmod(m_ten, x);

  EXPECT_EQ(e1, std::fmod((double)ten, (double)one));
  EXPECT_EQ(e2, std::fmod((double)ten, (double)m_one));
  EXPECT_EQ(e3, std::fmod((double)ten, (double)one_point_five));
  EXPECT_EQ(e4, std::fmod((double)ten, (double)m_one_point_five));
  EXPECT_EQ(e5, std::fmod((double)ten, (double)x));
  EXPECT_EQ(e6, std::fmod((double)m_ten, (double)x));
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
  EXPECT_TRUE(fp32::isNaN(fp32::Sqrt(-one)));

  fp32   step{0.01};
  fp32   x{tiny}, max{huge};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 4 * (double)fp32::TOLERANCE;
  for (; x < max; x += step)
  {
    fp32   e     = fp32::Sqrt(x);
    double r     = std::sqrt((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, 5 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Sqrt: max error = " << max_error << std::endl;
  std::cout << "Sqrt: avg error = " << avg_error << std::endl;
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
  EXPECT_TRUE(fp64::isNaN(fp64::Sqrt(-one)));

  fp64   step{0.001};
  fp64   x{tiny}, max{huge};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::Sqrt(x);
    double r     = std::sqrt((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, 10 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Sqrt: max error = " << max_error << std::endl;
  std::cout << "Sqrt: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Sin_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> huge(2000);
  fetch::fixed_point::FixedPoint<16, 16> small(0.0001);
  fetch::fixed_point::FixedPoint<16, 16> tiny(0, fp32::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> e1  = fp32::Sin(one);
  fetch::fixed_point::FixedPoint<16, 16> e2  = fp32::Sin(one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e3  = fp32::Sin(fp32::_0);
  fetch::fixed_point::FixedPoint<16, 16> e4  = fp32::Sin(huge);
  fetch::fixed_point::FixedPoint<16, 16> e5  = fp32::Sin(small);
  fetch::fixed_point::FixedPoint<16, 16> e6  = fp32::Sin(tiny);
  fetch::fixed_point::FixedPoint<16, 16> e7  = fp32::Sin(fp32::CONST_PI);
  fetch::fixed_point::FixedPoint<16, 16> e8  = fp32::Sin(-fp32::CONST_PI);
  fetch::fixed_point::FixedPoint<16, 16> e9  = fp32::Sin(fp32::CONST_PI * 2);
  fetch::fixed_point::FixedPoint<16, 16> e10 = fp32::Sin(fp32::CONST_PI * 4);
  fetch::fixed_point::FixedPoint<16, 16> e11 = fp32::Sin(fp32::CONST_PI * 100);
  fetch::fixed_point::FixedPoint<16, 16> e12 = fp32::Sin(fp32::CONST_PI_2);
  fetch::fixed_point::FixedPoint<16, 16> e13 = fp32::Sin(-fp32::CONST_PI_2);
  fetch::fixed_point::FixedPoint<16, 16> e14 = fp32::Sin(fp32::CONST_PI_4);
  fetch::fixed_point::FixedPoint<16, 16> e15 = fp32::Sin(-fp32::CONST_PI_4);
  fetch::fixed_point::FixedPoint<16, 16> e16 = fp32::Sin(fp32::CONST_PI_4 * 3);

  double delta = (double)e1 - std::sin((double)one);
  EXPECT_NEAR(delta / std::sin((double)one), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e2 - std::sin((double)one_point_five);
  EXPECT_NEAR(delta / std::sin((double)one_point_five), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e3 - std::sin((double)fp32::_0);
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e4 - std::sin((double)huge);
  EXPECT_NEAR(delta / std::sin((double)huge), 0.0,
              0.002);  // Sin for larger arguments loses precision
  delta = (double)e5 - std::sin((double)small);
  EXPECT_NEAR(delta / std::sin((double)small), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e6 - std::sin((double)tiny);
  EXPECT_NEAR(delta / std::sin((double)tiny), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e7 - std::sin((double)fp32::CONST_PI);
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e8 - std::sin((double)(-fp32::CONST_PI));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e9 - std::sin((double)(fp32::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e10 - std::sin((double)(fp32::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e11 - std::sin((double)(fp32::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = (double)e12 - std::sin((double)(fp32::CONST_PI_2));
  EXPECT_NEAR(delta / std::sin((double)(fp32::CONST_PI_2)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e13 - std::sin((double)(-fp32::CONST_PI_2));
  EXPECT_NEAR(delta / std::sin((double)(-fp32::CONST_PI_2)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e14 - std::sin((double)(fp32::CONST_PI_4));
  EXPECT_NEAR(delta / std::sin((double)(fp32::CONST_PI_4)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e15 - std::sin((double)(-fp32::CONST_PI_4));
  EXPECT_NEAR(delta / std::sin((double)(-fp32::CONST_PI_4)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e16 - std::sin((double)(fp32::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::sin((double)(fp32::CONST_PI_4 * 3)), 0.0, (double)fp32::TOLERANCE);

  fp32 step{0.1};
  fp32 x{-fp32::CONST_PI * 10};
  for (; x < fp32::CONST_PI * 10; x += step)
  {
    fp32 e = fp32::Sin(x);
    delta  = (double)e - std::sin((double)(x));
    EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  }
}

TEST(FixedPointTest, Sin_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> huge(20000000);
  fetch::fixed_point::FixedPoint<32, 32> small(0.0000001);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, fp32::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> e1  = fp64::Sin(one);
  fetch::fixed_point::FixedPoint<32, 32> e2  = fp64::Sin(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e3  = fp64::Sin(fp64::_0);
  fetch::fixed_point::FixedPoint<32, 32> e4  = fp64::Sin(huge);
  fetch::fixed_point::FixedPoint<32, 32> e5  = fp64::Sin(small);
  fetch::fixed_point::FixedPoint<32, 32> e6  = fp64::Sin(tiny);
  fetch::fixed_point::FixedPoint<32, 32> e7  = fp64::Sin(fp64::CONST_PI);
  fetch::fixed_point::FixedPoint<32, 32> e8  = fp64::Sin(-fp64::CONST_PI);
  fetch::fixed_point::FixedPoint<32, 32> e9  = fp64::Sin(fp64::CONST_PI * 2);
  fetch::fixed_point::FixedPoint<32, 32> e10 = fp64::Sin(fp64::CONST_PI * 4);
  fetch::fixed_point::FixedPoint<32, 32> e11 = fp64::Sin(fp64::CONST_PI * 100);
  fetch::fixed_point::FixedPoint<32, 32> e12 = fp64::Sin(fp64::CONST_PI_2);
  fetch::fixed_point::FixedPoint<32, 32> e13 = fp64::Sin(-fp64::CONST_PI_2);
  fetch::fixed_point::FixedPoint<32, 32> e14 = fp64::Sin(fp64::CONST_PI_4);
  fetch::fixed_point::FixedPoint<32, 32> e15 = fp64::Sin(-fp64::CONST_PI_4);
  fetch::fixed_point::FixedPoint<32, 32> e16 = fp64::Sin(fp64::CONST_PI_4 * 3);

  double delta = (double)e1 - std::sin((double)one);
  EXPECT_NEAR(delta / std::sin((double)one), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e2 - std::sin((double)one_point_five);
  EXPECT_NEAR(delta / std::sin((double)one_point_five), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e3 - std::sin((double)fp64::_0);
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e4 - std::sin((double)huge);
  EXPECT_NEAR(delta / std::sin((double)huge), 0.0,
              0.001);  // Sin for larger arguments loses precision
  delta = (double)e5 - std::sin((double)small);
  EXPECT_NEAR(delta / std::sin((double)small), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e6 - std::sin((double)tiny);
  EXPECT_NEAR(delta / std::sin((double)tiny), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e7 - std::sin((double)fp64::CONST_PI);
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e8 - std::sin((double)(-fp64::CONST_PI));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e9 - std::sin((double)(fp64::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e10 - std::sin((double)(fp64::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e11 - std::sin((double)(fp64::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);  // Sin for larger arguments loses precision
  delta = (double)e12 - std::sin((double)(fp64::CONST_PI_2));
  EXPECT_NEAR(delta / std::sin((double)(fp64::CONST_PI_2)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e13 - std::sin((double)(-fp64::CONST_PI_2));
  EXPECT_NEAR(delta / std::sin((double)(-fp64::CONST_PI_2)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e14 - std::sin((double)(fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::sin((double)(fp64::CONST_PI_4)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e15 - std::sin((double)(-fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::sin((double)(-fp64::CONST_PI_4)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e16 - std::sin((double)(fp64::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::sin((double)(fp64::CONST_PI_4 * 3)), 0.0, (double)fp64::TOLERANCE);

  fp64 step{0.001};
  fp64 x{-fp64::CONST_PI * 100};
  for (; x < fp64::CONST_PI * 100; x += step)
  {
    fp64 e = fp64::Sin(x);
    delta  = (double)e - std::sin((double)(x));
    EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  }
}

TEST(FixedPointTest, Cos_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> huge(2000);
  fetch::fixed_point::FixedPoint<16, 16> small(0.0001);
  fetch::fixed_point::FixedPoint<16, 16> tiny(0, fp32::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> e1  = fp32::Cos(one);
  fetch::fixed_point::FixedPoint<16, 16> e2  = fp32::Cos(one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e3  = fp32::Cos(fp32::_0);
  fetch::fixed_point::FixedPoint<16, 16> e4  = fp32::Cos(huge);
  fetch::fixed_point::FixedPoint<16, 16> e5  = fp32::Cos(small);
  fetch::fixed_point::FixedPoint<16, 16> e6  = fp32::Cos(tiny);
  fetch::fixed_point::FixedPoint<16, 16> e7  = fp32::Cos(fp32::CONST_PI);
  fetch::fixed_point::FixedPoint<16, 16> e8  = fp32::Cos(-fp32::CONST_PI);
  fetch::fixed_point::FixedPoint<16, 16> e9  = fp32::Cos(fp32::CONST_PI * 2);
  fetch::fixed_point::FixedPoint<16, 16> e10 = fp32::Cos(fp32::CONST_PI * 4);
  fetch::fixed_point::FixedPoint<16, 16> e11 = fp32::Cos(fp32::CONST_PI * 100);
  fetch::fixed_point::FixedPoint<16, 16> e12 = fp32::Cos(fp32::CONST_PI_2);
  fetch::fixed_point::FixedPoint<16, 16> e13 = fp32::Cos(-fp32::CONST_PI_2);
  fetch::fixed_point::FixedPoint<16, 16> e14 = fp32::Cos(fp32::CONST_PI_4);
  fetch::fixed_point::FixedPoint<16, 16> e15 = fp32::Cos(-fp32::CONST_PI_4);
  fetch::fixed_point::FixedPoint<16, 16> e16 = fp32::Cos(fp32::CONST_PI_4 * 3);

  double delta = (double)e1 - std::cos((double)one);
  EXPECT_NEAR(delta / std::cos((double)one), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e2 - std::cos((double)one_point_five);
  EXPECT_NEAR(delta / std::cos((double)one_point_five), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e3 - std::cos((double)fp32::_0);
  EXPECT_NEAR(delta / std::cos((double)fp32::_0), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e4 - std::cos((double)huge);
  EXPECT_NEAR(delta / std::cos((double)huge), 0.0,
              0.012);  // Sin for larger arguments loses precision
  delta = (double)e5 - std::cos((double)small);
  EXPECT_NEAR(delta / std::cos((double)small), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e6 - std::cos((double)tiny);
  EXPECT_NEAR(delta / std::cos((double)tiny), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e7 - std::cos((double)fp64::CONST_PI);
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e8 - std::cos((double)(-fp64::CONST_PI));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e9 - std::cos((double)(fp64::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e10 - std::cos((double)(fp64::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e11 - std::cos((double)(fp64::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = (double)e12 - std::cos((double)(fp64::CONST_PI_2));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e13 - std::cos((double)(-fp64::CONST_PI_2));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e14 - std::cos((double)(fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::cos((double)(fp64::CONST_PI_4)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e15 - std::cos((double)(-fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::cos((double)(-fp64::CONST_PI_4)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e16 - std::cos((double)(fp64::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::cos((double)(fp64::CONST_PI_4 * 3)), 0.0, (double)fp32::TOLERANCE);

  fp32 step{0.1};
  fp32 x{-fp32::CONST_PI * 10};
  for (; x < fp32::CONST_PI * 10; x += step)
  {
    fp32 e = fp32::Cos(x);
    delta  = (double)e - std::cos((double)(x));
    EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  }
}

TEST(FixedPointTest, Cos_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> huge(2000);
  fetch::fixed_point::FixedPoint<32, 32> small(0.0001);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, fp32::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> e1  = fp64::Cos(one);
  fetch::fixed_point::FixedPoint<32, 32> e2  = fp64::Cos(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e3  = fp64::Cos(fp64::_0);
  fetch::fixed_point::FixedPoint<32, 32> e4  = fp64::Cos(huge);
  fetch::fixed_point::FixedPoint<32, 32> e5  = fp64::Cos(small);
  fetch::fixed_point::FixedPoint<32, 32> e6  = fp64::Cos(tiny);
  fetch::fixed_point::FixedPoint<32, 32> e7  = fp64::Cos(fp64::CONST_PI);
  fetch::fixed_point::FixedPoint<32, 32> e8  = fp64::Cos(-fp64::CONST_PI);
  fetch::fixed_point::FixedPoint<32, 32> e9  = fp64::Cos(fp64::CONST_PI * 2);
  fetch::fixed_point::FixedPoint<32, 32> e10 = fp64::Cos(fp64::CONST_PI * 4);
  fetch::fixed_point::FixedPoint<32, 32> e11 = fp64::Cos(fp64::CONST_PI * 100);
  fetch::fixed_point::FixedPoint<32, 32> e12 = fp64::Cos(fp64::CONST_PI_2);
  fetch::fixed_point::FixedPoint<32, 32> e13 = fp64::Cos(-fp64::CONST_PI_2);
  fetch::fixed_point::FixedPoint<32, 32> e14 = fp64::Cos(fp64::CONST_PI_4);
  fetch::fixed_point::FixedPoint<32, 32> e15 = fp64::Cos(-fp64::CONST_PI_4);
  fetch::fixed_point::FixedPoint<32, 32> e16 = fp64::Cos(fp64::CONST_PI_4 * 3);

  double delta = (double)e1 - std::cos((double)one);
  EXPECT_NEAR(delta / std::cos((double)one), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e2 - std::cos((double)one_point_five);
  EXPECT_NEAR(delta / std::cos((double)one_point_five), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e3 - std::cos((double)fp64::_0);
  EXPECT_NEAR(delta / std::cos((double)fp64::_0), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e4 - std::cos((double)huge);
  EXPECT_NEAR(delta / std::cos((double)huge), 0.0,
              0.002);  // Sin for larger arguments loses precision
  delta = (double)e5 - std::cos((double)small);
  EXPECT_NEAR(delta / std::cos((double)small), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e6 - std::cos((double)tiny);
  EXPECT_NEAR(delta / std::cos((double)tiny), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e7 - std::cos((double)fp64::CONST_PI);
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e8 - std::cos((double)(-fp64::CONST_PI));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e9 - std::cos((double)(fp64::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e10 - std::cos((double)(fp64::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e11 - std::cos((double)(fp64::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = (double)e12 - std::cos((double)(fp64::CONST_PI_2));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e13 - std::cos((double)(-fp64::CONST_PI_2));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e14 - std::cos((double)(fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::cos((double)(fp64::CONST_PI_4)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e15 - std::cos((double)(-fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::cos((double)(-fp64::CONST_PI_4)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e16 - std::cos((double)(fp64::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::cos((double)(fp64::CONST_PI_4 * 3)), 0.0, (double)fp64::TOLERANCE);

  fp64 step{0.1};
  fp64 x{-fp64::CONST_PI * 100};
  for (; x < fp64::CONST_PI * 100; x += step)
  {
    fp64 e = fp64::Cos(x);
    delta  = (double)e - std::cos((double)(x));
    EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  }
}

TEST(FixedPointTest, Tan_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> huge(2000);
  fetch::fixed_point::FixedPoint<16, 16> small(0.0001);
  fetch::fixed_point::FixedPoint<16, 16> tiny(0, fp32::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<16, 16> e1  = fp32::Tan(one);
  fetch::fixed_point::FixedPoint<16, 16> e2  = fp32::Tan(one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e3  = fp32::Tan(fp32::_0);
  fetch::fixed_point::FixedPoint<16, 16> e4  = fp32::Tan(huge);
  fetch::fixed_point::FixedPoint<16, 16> e5  = fp32::Tan(small);
  fetch::fixed_point::FixedPoint<16, 16> e6  = fp32::Tan(tiny);
  fetch::fixed_point::FixedPoint<16, 16> e7  = fp32::Tan(fp32::CONST_PI);
  fetch::fixed_point::FixedPoint<16, 16> e8  = fp32::Tan(-fp32::CONST_PI);
  fetch::fixed_point::FixedPoint<16, 16> e9  = fp32::Tan(fp32::CONST_PI * 2);
  fetch::fixed_point::FixedPoint<16, 16> e10 = fp32::Tan(fp32::CONST_PI * 4);
  fetch::fixed_point::FixedPoint<16, 16> e11 = fp32::Tan(fp32::CONST_PI * 100);
  fetch::fixed_point::FixedPoint<16, 16> e12 = fp32::Tan(fp32::CONST_PI_4);
  fetch::fixed_point::FixedPoint<16, 16> e13 = fp32::Tan(-fp32::CONST_PI_4);
  fetch::fixed_point::FixedPoint<16, 16> e14 = fp32::Tan(fp32::CONST_PI_4 * 3);

  double delta = (double)e1 - std::tan((double)one);
  EXPECT_NEAR(delta / std::tan((double)one), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e2 - std::tan((double)one_point_five);
  EXPECT_NEAR(delta / std::tan((double)one_point_five), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e3 - std::tan((double)fp32::_0);
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e4 - std::tan((double)huge);
  // Tan for larger arguments loses precision
  EXPECT_NEAR(delta / std::tan((double)huge), 0.0, 0.012);
  delta = (double)e5 - std::tan((double)small);
  EXPECT_NEAR(delta / std::tan((double)small), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e6 - std::tan((double)tiny);
  EXPECT_NEAR(delta / std::tan((double)tiny), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e7 - std::tan((double)fp64::CONST_PI);
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e8 - std::tan((double)(-fp64::CONST_PI));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e9 - std::tan((double)(fp64::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e10 - std::tan((double)(fp64::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, (double)fp32::TOLERANCE);
  delta = (double)e11 - std::tan((double)(fp64::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = (double)e12 - std::tan((double)(fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::tan((double)(fp64::CONST_PI_4)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e13 - std::tan((double)(-fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::tan((double)(-fp64::CONST_PI_4)), 0.0, (double)fp32::TOLERANCE);
  delta = (double)e14 - std::tan((double)(fp64::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::tan((double)(fp64::CONST_PI_4 * 3)), 0.0, (double)fp32::TOLERANCE);

  EXPECT_TRUE(fp32::isPosInfinity(fp32::Tan(fp32::CONST_PI_2)));
  EXPECT_TRUE(fp32::isNegInfinity(fp32::Tan(-fp32::CONST_PI_2)));

  fp32 step{0.001}, offset{step * 10};
  fp32 x{-fp32::CONST_PI_2}, max{fp32::CONST_PI_2};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp32::TOLERANCE;
  for (; x < max; x += step)
  {
    fp32   e     = fp32::Tan(x);
    double r     = std::tan((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  // EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Tan: max error = " << max_error << std::endl;
  std::cout << "Tan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Tan_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> huge(2000);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, fp32::SMALLEST_FRACTION);
  fetch::fixed_point::FixedPoint<32, 32> e1  = fp64::Tan(one);
  fetch::fixed_point::FixedPoint<32, 32> e2  = fp64::Tan(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e3  = fp64::Tan(fp64::_0);
  fetch::fixed_point::FixedPoint<32, 32> e4  = fp64::Tan(huge);
  fetch::fixed_point::FixedPoint<32, 32> e5  = fp64::Tan(tiny);
  fetch::fixed_point::FixedPoint<32, 32> e6  = fp64::Tan(fp64::CONST_PI);
  fetch::fixed_point::FixedPoint<32, 32> e7  = fp64::Tan(-fp64::CONST_PI);
  fetch::fixed_point::FixedPoint<32, 32> e8  = fp64::Tan(fp64::CONST_PI * 2);
  fetch::fixed_point::FixedPoint<32, 32> e9  = fp64::Tan(fp64::CONST_PI * 4);
  fetch::fixed_point::FixedPoint<32, 32> e10 = fp64::Tan(fp64::CONST_PI * 100);
  fetch::fixed_point::FixedPoint<32, 32> e11 = fp64::Tan(fp64::CONST_PI_4);
  fetch::fixed_point::FixedPoint<32, 32> e12 = fp64::Tan(-fp64::CONST_PI_4);
  fetch::fixed_point::FixedPoint<32, 32> e13 = fp64::Tan(fp64::CONST_PI_4 * 3);

  double delta = (double)e1 - std::tan((double)one);
  EXPECT_NEAR(delta / std::tan((double)one), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e2 - std::tan((double)one_point_five);
  EXPECT_NEAR(delta / std::tan((double)one_point_five), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e3 - std::tan((double)fp64::_0);
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e4 - std::tan((double)huge);
  EXPECT_NEAR(delta / std::tan((double)huge), 0.0, 0.012);
  delta = (double)e5 - std::tan((double)tiny);
  EXPECT_NEAR(delta / std::tan((double)tiny), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e6 - std::tan((double)fp64::CONST_PI);
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e7 - std::tan((double)(-fp64::CONST_PI));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e8 - std::tan((double)(fp64::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e9 - std::tan((double)(fp64::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, (double)fp64::TOLERANCE);
  delta = (double)e10 - std::tan((double)(fp64::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = (double)e11 - std::tan((double)(fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::tan((double)(fp64::CONST_PI_4)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e12 - std::tan((double)(-fp64::CONST_PI_4));
  EXPECT_NEAR(delta / std::tan((double)(-fp64::CONST_PI_4)), 0.0, (double)fp64::TOLERANCE);
  delta = (double)e13 - std::tan((double)(fp64::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::tan((double)(fp64::CONST_PI_4 * 3)), 0.0, (double)fp64::TOLERANCE);

  // (843, private) Replace with IsInfinity()
  EXPECT_TRUE(fp64::isPosInfinity(fp64::Tan(fp64::CONST_PI_2)));
  EXPECT_TRUE(fp64::isNegInfinity(fp64::Tan(-fp64::CONST_PI_2)));

  fp64 step{0.0001}, offset{step * 100};
  fp64 x{-fp64::CONST_PI_2}, max{fp64::CONST_PI_2};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp64::TOLERANCE;
  for (; x < max; x += step)
  {
    fp64   e     = fp64::Tan(x);
    double r     = std::tan((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  // EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "Tan: max error = " << max_error << std::endl;
  std::cout << "Tan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASin_16_16)
{
  fp32   step{0.001};
  fp32   x{-0.99};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 1.0; x += step)
  {
    fp32   e     = fp32::ASin(x);
    double r     = std::asin((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ASin: max error = " << max_error << std::endl;
  std::cout << "ASin: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASin_32_32)
{
  fp64   step{0.0001};
  fp64   x{-0.999};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 1.0; x += step)
  {
    fp64   e     = fp64::ASin(x);
    double r     = std::asin((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ASin: max error = " << max_error << std::endl;
  std::cout << "ASin: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACos_16_16)
{
  fp32   step{0.001};
  fp32   x{-0.99};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 1.0; x += step)
  {
    fp32   e     = fp32::ACos(x);
    double r     = std::acos((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ACos: max error = " << max_error << std::endl;
  std::cout << "ACos: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACos_32_32)
{
  fp64   step{0.0001};
  fp64   x{-1.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 1.0; x += step)
  {
    fp64   e     = fp64::ACos(x);
    double r     = std::acos((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ACos: max error = " << max_error << std::endl;
  std::cout << "ACos: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan_16_16)
{
  fp32   step{0.001};
  fp32   x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp32   e     = fp32::ATan(x);
    double r     = std::atan((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ATan: max error = " << max_error << std::endl;
  std::cout << "ATan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan_32_32)
{
  fp64   step{0.0001};
  fp64   x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::ATan(x);
    double r     = std::atan((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ATan: max error = " << max_error << std::endl;
  std::cout << "ATan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan2_16_16)
{
  fp32   step{0.01};
  fp32   x{-2.0};
  fp32   y{-2.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 2.0; x += step)
  {
    for (; y < 2.0; y += step)
    {
      fp32   e     = fp32::ATan2(y, x);
      double r     = std::atan2((double)y, (double)x);
      double delta = std::abs((double)e - r);
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ATan2: max error = " << max_error << std::endl;
  std::cout << "ATan2: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan2_32_32)
{
  fp64   step{0.0001};
  fp64   x{-2.0};
  fp64   y{-2.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 2.0; x += step)
  {
    for (; y < 2.0; y += step)
    {
      fp64   e     = fp64::ATan2(y, x);
      double r     = std::atan2((double)y, (double)x);
      double delta = std::abs((double)e - r);
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ATan2: max error = " << max_error << std::endl;
  std::cout << "ATan2: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, SinH_16_16)
{
  fp32   step{0.001};
  fp32   x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2.0 * (double)fp32::TOLERANCE;
  for (; x < 3.0; x += step)
  {
    fp32   e     = fp32::SinH(x);
    double r     = std::sinh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "SinH: max error = " << max_error << std::endl;
  std::cout << "SinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, SinH_32_32)
{
  fp64   step{0.0001};
  fp64   x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::SinH(x);
    double r     = std::sinh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "SinH: max error = " << max_error << std::endl;
  std::cout << "SinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, CosH_16_16)
{
  fp32   step{0.001};
  fp32   x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2.0 * (double)fp32::TOLERANCE;
  for (; x < 3.0; x += step)
  {
    fp32   e     = fp32::CosH(x);
    double r     = std::cosh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "CosH: max error = " << max_error << std::endl;
  std::cout << "CosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, CosH_32_32)
{
  fp64   step{0.0001};
  fp64   x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::CosH(x);
    double r     = std::cosh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "CosH: max error = " << max_error << std::endl;
  std::cout << "CosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, TanH_16_16)
{
  fp32   step{0.001};
  fp32   x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp32::TOLERANCE;
  for (; x < 3.0; x += step)
  {
    fp32   e     = fp32::TanH(x);
    double r     = std::tanh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "TanH: max error = " << max_error << std::endl;
  std::cout << "TanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, TanH_32_32)
{
  fp64   step{0.0001};
  fp64   x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::TanH(x);
    double r     = std::tanh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "TanH: max error = " << max_error << std::endl;
  std::cout << "TanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASinH_16_16)
{
  fp32   step{0.001};
  fp32   x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp32::TOLERANCE;
  for (; x < 3.0; x += step)
  {
    fp32   e     = fp32::ASinH(x);
    double r     = std::asinh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ASinH: max error = " << max_error << std::endl;
  std::cout << "ASinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASinH_32_32)
{
  fp64   step{0.0001};
  fp64   x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::ASinH(x);
    double r     = std::asinh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ASinH: max error = " << max_error << std::endl;
  std::cout << "ASinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACosH_16_16)
{
  fp32   step{0.001};
  fp32   x{1.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp32::TOLERANCE;
  for (; x < 3.0; x += step)
  {
    fp32   e     = fp32::ACosH(x);
    double r     = std::acosh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ACosH: max error = " << max_error << std::endl;
  std::cout << "ACosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACosH_32_32)
{
  fp64   step{0.0001};
  fp64   x{1.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp64::TOLERANCE;
  for (; x < 5.0; x += step)
  {
    fp64   e     = fp64::ACosH(x);
    double r     = std::acosh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ACosH: max error = " << max_error << std::endl;
  std::cout << "ACosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATanH_16_16)
{
  fp32 step{0.001}, offset{step * 10};
  fp32 x{-1.0}, max{1.0};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp32::TOLERANCE;
  for (; x < max; x += step)
  {
    fp32   e     = fp32::ATanH(x);
    double r     = std::atanh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, 2 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ATamH: max error = " << max_error << std::endl;
  std::cout << "ATanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATanH_32_32)
{
  fp64 step{0.0001}, offset{step * 10};
  fp64 x{-1.0}, max{1.0};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * (double)fp64::TOLERANCE;
  for (; x < max; x += step)
  {
    fp64   e     = fp64::ATanH(x);
    double r     = std::atanh((double)x);
    double delta = std::abs((double)e - r);
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= (double)iterations;
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  std::cout << "ATanH: max error = " << max_error << std::endl;
  std::cout << "ATanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, NanInfinity_16_16)
{
  std::cout << fp32::NaN << std::endl;
  std::cout << fp32::POSITIVE_INFINITY << std::endl;
  std::cout << fp32::NEGATIVE_INFINITY << std::endl;

  fp32 m_inf{fp32::NEGATIVE_INFINITY};
  fp32 p_inf{fp32::POSITIVE_INFINITY};

  // Basic checks
  EXPECT_TRUE(fp32::isInfinity(m_inf));
  EXPECT_TRUE(fp32::isNegInfinity(m_inf));
  EXPECT_TRUE(fp32::isInfinity(p_inf));
  EXPECT_TRUE(fp32::isPosInfinity(p_inf));
  EXPECT_FALSE(fp32::isNegInfinity(p_inf));
  EXPECT_FALSE(fp32::isPosInfinity(m_inf));

  // Addition checks
  EXPECT_TRUE(fp32::isNegInfinity(m_inf + m_inf));
  EXPECT_TRUE(fp32::isPosInfinity(p_inf + p_inf));
  EXPECT_TRUE(fp32::isNaN(m_inf + p_inf));
  EXPECT_TRUE(fp32::isNaN(p_inf + m_inf));

  // Subtraction checks
  // Multiplication checks
  // Division checks
  // Exponential checks
  // Logarithm checks
  // Trigonometry checks
}

TEST(FixedPointTest, NanInfinity_32_32)
{
  std::cout << fp64::NaN << std::endl;
  std::cout << fp64::POSITIVE_INFINITY << std::endl;
  std::cout << fp64::NEGATIVE_INFINITY << std::endl;

  fp64 m_inf{fp64::NEGATIVE_INFINITY};
  fp64 p_inf{fp64::POSITIVE_INFINITY};

  // Basic checks
  EXPECT_TRUE(fp64::isInfinity(m_inf));
  EXPECT_TRUE(fp64::isNegInfinity(m_inf));
  EXPECT_TRUE(fp64::isInfinity(p_inf));
  EXPECT_TRUE(fp64::isPosInfinity(p_inf));
  EXPECT_FALSE(fp64::isNegInfinity(p_inf));
  EXPECT_FALSE(fp64::isPosInfinity(m_inf));
}
