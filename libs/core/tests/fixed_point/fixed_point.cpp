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

#include "core/fixed_point/fixed_point.hpp"
#include <array>
#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>
#include <limits>
#include <cmath>

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

  // Zero
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
  EXPECT_EQ(one.Data(),             0x10000);
  EXPECT_EQ(one_point_five.Data(),  0x18000);
  EXPECT_EQ(two_point_five.Data(),  0x28000);

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
  fetch::fixed_point::FixedPoint<16, 16> smallest_fixed_point = smallest_int - almost_one;

  EXPECT_EQ(infinitesimal.Data(),        infinitesimal.smallest_fraction);
  EXPECT_EQ(almost_one.Data(),           almost_one.largest_fraction);
  EXPECT_EQ(largest_int.Data(),          largest_int.largest_int);
  EXPECT_EQ(smallest_int.Data(),         smallest_int.smallest_int);
  EXPECT_EQ(largest_fixed_point.Data(),  largest_fixed_point.max);
  EXPECT_EQ(smallest_fixed_point.Data(), smallest_fixed_point.min);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int32_t>::min());
  // On the other hand we expect to be exactly the same as the largest positive integer of int64_t
  EXPECT_TRUE(largest_fixed_point.Data() == std::numeric_limits<int32_t>::max());
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

  // Zero
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
  EXPECT_EQ(one.Data(),             0x100000000);
  EXPECT_EQ(one_point_five.Data(),  0x180000000);
  EXPECT_EQ(two_point_five.Data(),  0x280000000);

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
  fetch::fixed_point::FixedPoint<32, 32> smallest_fixed_point = smallest_int - almost_one;

  EXPECT_EQ(infinitesimal.Data(),        infinitesimal.smallest_fraction);
  EXPECT_EQ(almost_one.Data(),           almost_one.largest_fraction);
  EXPECT_EQ(largest_int.Data(),          largest_int.largest_int);
  EXPECT_EQ(smallest_int.Data(),         smallest_int.smallest_int);
  EXPECT_EQ(largest_fixed_point.Data(),  largest_fixed_point.max);
  EXPECT_EQ(smallest_fixed_point.Data(), smallest_fixed_point.min);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int64_t>::min());
  // On the other hand we expect to be exactly the same as the largest positive integer of int64_t
  EXPECT_TRUE(largest_fixed_point.Data() == std::numeric_limits<int64_t>::max());
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

  // Zero
  fetch::fixed_point::FixedPoint<16, 16> zero(0);
  fetch::fixed_point::FixedPoint<16, 16> m_zero(-0);

  EXPECT_EQ(int(zero), 0);
  EXPECT_EQ(int(m_zero), 0);
  EXPECT_EQ(float(zero), 0.0f);
  EXPECT_EQ(float(m_zero), 0.0f);
  EXPECT_EQ(double(zero), 0.0);
  EXPECT_EQ(double(m_zero), 0.0);

  // Infinitesimal additions
  fetch::fixed_point::FixedPoint<16, 16> almost_one(0, one.largest_fraction);
  fetch::fixed_point::FixedPoint<16, 16> infinitesimal(0, one.smallest_fraction);

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

  // Zero
  fetch::fixed_point::FixedPoint<32, 32> zero(0);
  fetch::fixed_point::FixedPoint<32, 32> m_zero(-0);

  EXPECT_EQ(int(zero), 0);
  EXPECT_EQ(int(m_zero), 0);
  EXPECT_EQ(float(zero), 0.0f);
  EXPECT_EQ(float(m_zero), 0.0f);
  EXPECT_EQ(double(zero), 0.0);
  EXPECT_EQ(double(m_zero), 0.0);

  // Infinitesimal additions
  fetch::fixed_point::FixedPoint<32, 32> almost_one(0, one.largest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> infinitesimal(0, one.smallest_fraction);

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
  fetch::fixed_point::FixedPoint<16, 16> almost_three(2, one.largest_fraction);
  fetch::fixed_point::FixedPoint<16, 16> almost_two(1, one.largest_fraction);

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
  fetch::fixed_point::FixedPoint<32, 32> almost_three(2, one.largest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> almost_two(1, one.largest_fraction);

  EXPECT_EQ(almost_three - almost_two, one);
}

TEST(FixedPointTest, Multiplication_16_16)
{
  // Positive
  fetch::fixed_point::FixedPoint<16, 16> zero(0);
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);
  fetch::fixed_point::FixedPoint<16, 16> three(3);

  EXPECT_EQ(two * one, two);
  EXPECT_EQ(one * 2, 2);
  EXPECT_EQ(float(two * 2.0f), 4.0f);
  EXPECT_EQ(double(three * 2.0), 6.0);

  EXPECT_EQ(int(one * two), 2);
  EXPECT_EQ(float(one * two), 2.0f);
  EXPECT_EQ(double(one * two), 2.0);

  EXPECT_EQ(int(two * zero), 0);
  EXPECT_EQ(float(two * zero), 0.0f);
  EXPECT_EQ(double(two * zero), 0.0);

  // Extreme cases
  fetch::fixed_point::FixedPoint<16, 16> almost_one(0, one.largest_fraction);
  fetch::fixed_point::FixedPoint<16, 16> infinitesimal(0, one.smallest_fraction);
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

  EXPECT_EQ(two * one, two);
  EXPECT_EQ(one * 2, 2);
  EXPECT_EQ(float(two * 2.0f), 4.0f);
  EXPECT_EQ(double(three * 2.0), 6.0);

  EXPECT_EQ(int(one * two), 2);
  EXPECT_EQ(float(one * two), 2.0f);
  EXPECT_EQ(double(one * two), 2.0);

  EXPECT_EQ(int(two * zero), 0);
  EXPECT_EQ(float(two * zero), 0.0f);
  EXPECT_EQ(double(two * zero), 0.0);

  // Extreme cases
  fetch::fixed_point::FixedPoint<32, 32> almost_one(0, one.largest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> infinitesimal(0, one.smallest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> huge(0x40000000, 0);
  fetch::fixed_point::FixedPoint<32, 32> small(0, 0x40000000);

  EXPECT_EQ(almost_one * almost_one, almost_one - infinitesimal);
  EXPECT_EQ(almost_one * infinitesimal, zero);
  EXPECT_EQ(huge * infinitesimal, small);

  // (One of) largest possible multiplications
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
  fetch::fixed_point::FixedPoint<16, 16> almost_one(0, one.largest_fraction);
  fetch::fixed_point::FixedPoint<16, 16> infinitesimal(0, one.smallest_fraction);
  fetch::fixed_point::FixedPoint<16, 16> huge(0x4000, 0);
  fetch::fixed_point::FixedPoint<16, 16> small(0, 0x4000);

  EXPECT_EQ(small / infinitesimal, huge);
  EXPECT_EQ(infinitesimal / one, infinitesimal);
  EXPECT_EQ(one / huge, infinitesimal * 4);
  EXPECT_EQ(huge / infinitesimal, zero);

  // TODO(843): Add proper support for NaN/Infinity/etc in FixedPoint class
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
  fetch::fixed_point::FixedPoint<32, 32> almost_one(0, one.largest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> infinitesimal(0, one.smallest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> huge(0x40000000, 0);
  fetch::fixed_point::FixedPoint<32, 32> small(0, 0x40000000);

  EXPECT_EQ(small / infinitesimal, huge);
  EXPECT_EQ(infinitesimal / one, infinitesimal);
  EXPECT_EQ(one / huge, infinitesimal * 4);
  EXPECT_EQ(huge / infinitesimal, zero);

  // TODO(843): Add proper support for NaN/Infinity/etc in FixedPoint class
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

  EXPECT_TRUE(fp32::E       == 2.718281828459045235360287471352662498);
  EXPECT_TRUE(fp32::LOG2E   == 1.442695040888963407359924681001892137);
  EXPECT_TRUE(fp32::LOG10E  == 0.434294481903251827651128918916605082);
  EXPECT_TRUE(fp32::LN2     == 0.693147180559945309417232121458176568);
  EXPECT_TRUE(fp32::LN10    == 2.302585092994045684017991454684364208);
  EXPECT_TRUE(fp32::PI      == 3.141592653589793238462643383279502884);
  EXPECT_TRUE(fp32::PI_2    == 1.570796326794896619231321691639751442);
  EXPECT_TRUE(fp32::PI_4    == 0.785398163397448309615660845819875721);
  EXPECT_TRUE(fp32::INV_PI  == 0.318309886183790671537767526745028724);
  EXPECT_TRUE(fp32::INV2_PI == 0.636619772367581343075535053490057448);
  EXPECT_TRUE(fp32::INV2_SQRTPI == 1.128379167095512573896158903121545172);
  EXPECT_TRUE(fp32::SQRT2   == 1.414213562373095048801688724209698079);
  EXPECT_TRUE(fp32::INV_SQRT2 == 0.707106781186547524400844362104849039);
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

  EXPECT_TRUE(m_two < one);
  EXPECT_TRUE(m_one < two);

  EXPECT_TRUE(fp64::E       == 2.718281828459045235360287471352662498);
  EXPECT_TRUE(fp64::LOG2E   == 1.442695040888963407359924681001892137);
  EXPECT_TRUE(fp64::LOG10E  == 0.434294481903251827651128918916605082);
  EXPECT_TRUE(fp64::LN2     == 0.693147180559945309417232121458176568);
  EXPECT_TRUE(fp64::LN10    == 2.302585092994045684017991454684364208);
  EXPECT_TRUE(fp64::PI      == 3.141592653589793238462643383279502884);
  EXPECT_TRUE(fp64::PI / 2 == fp64::PI_2);
  EXPECT_TRUE(fp64::PI_4    == 0.785398163397448309615660845819875721);
  EXPECT_TRUE(one / fp64::PI == fp64::INV_PI);
  EXPECT_TRUE(fp64::INV2_PI == 0.636619772367581343075535053490057448);
  EXPECT_TRUE(fp64::INV2_SQRTPI == 1.128379167095512573896158903121545172);
  EXPECT_TRUE(fp64::SQRT2   == 1.414213562373095048801688724209698079);
  EXPECT_TRUE(fp64::INV_SQRT2 == 0.707106781186547524400844362104849039);
}

TEST(FixedPointTest, Exponential_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> two(2);
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Exp(one);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Exp(two);

  EXPECT_NEAR((double)e1/std::exp(1.0), 1.0, 2e-5);
  EXPECT_NEAR((double)e2/std::exp(2.0), 1.0, 2e-5);
}

TEST(FixedPointTest, Exponential_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> huge(21);
  fetch::fixed_point::FixedPoint<32, 32> small(0.0001);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, one.smallest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Exp(one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Exp(two);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Exp(ten);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Exp(huge);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Exp(small);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Exp(tiny);

  EXPECT_NEAR((double)e1/std::exp(1.0), 1.0, 1e-7);
  EXPECT_NEAR((double)e2/std::exp(2.0), 1.0, 1e-7);
  EXPECT_NEAR((double)e3/std::exp(10.0), 1.0, 1e-7);
  EXPECT_NEAR((double)e4/std::exp(21), 1.0, 1e-7);
  EXPECT_NEAR((double)e5/std::exp(0.0001), 1.0, 1e-7);
  EXPECT_NEAR((double)e6/std::exp(double(tiny)), 1.0, 1e-7);
}

TEST(FixedPointTest, Logarithm_16_16)
{
  fetch::fixed_point::FixedPoint<16, 16> one(1);
  fetch::fixed_point::FixedPoint<16, 16> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<16, 16> ten(10);
  fetch::fixed_point::FixedPoint<16, 16> huge(10000);
  fetch::fixed_point::FixedPoint<16, 16> small(0.001);
  fetch::fixed_point::FixedPoint<16, 16> tiny(0, one.smallest_fraction);
  fetch::fixed_point::FixedPoint<16, 16> e1 = fp32::Log2(one);
  fetch::fixed_point::FixedPoint<16, 16> e2 = fp32::Log2(one_point_five);
  fetch::fixed_point::FixedPoint<16, 16> e3 = fp32::Log2(ten);
  fetch::fixed_point::FixedPoint<16, 16> e4 = fp32::Log2(huge);
  fetch::fixed_point::FixedPoint<16, 16> e5 = fp32::Log2(small);
  fetch::fixed_point::FixedPoint<16, 16> e6 = fp32::Log2(tiny);

  EXPECT_NEAR((double)e1, std::log2((double)one), 1e-4);
  EXPECT_NEAR((double)e2, std::log2((double)one_point_five), 1e-4);
  EXPECT_NEAR((double)e3, std::log2((double)ten), 1e-4);
  EXPECT_NEAR((double)e4, std::log2((double)huge), 1e-4);
  EXPECT_NEAR((double)e5, std::log2((double)small), 1e-4);
  EXPECT_NEAR((double)e6, std::log2((double)tiny), 1e-4);
}

TEST(FixedPointTest, Logarithm_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> huge(1000000000);
  fetch::fixed_point::FixedPoint<32, 32> small(0.0001);
  fetch::fixed_point::FixedPoint<32, 32> tiny(0, one.smallest_fraction);
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Log2(one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Log2(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Log2(ten);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Log2(huge);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Log2(small);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Log2(tiny);

  EXPECT_NEAR((double)e1, std::log2((double)one), 1e-7);
  EXPECT_NEAR((double)e2, std::log2((double)one_point_five), 1e-7);
  EXPECT_NEAR((double)e3, std::log2((double)ten), 1e-7);
  EXPECT_NEAR((double)e4, std::log2((double)huge), 1e-7);
  EXPECT_NEAR((double)e5, std::log2((double)small), 1e-7);
  EXPECT_NEAR((double)e6, std::log2((double)tiny), 1e-7);
}

TEST(FixedPointTest, Abs_32_32)
{
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> m_one(-1);
  fetch::fixed_point::FixedPoint<32, 32> one_point_five(1.5);
  fetch::fixed_point::FixedPoint<32, 32> m_one_point_five(-1.5);
  fetch::fixed_point::FixedPoint<32, 32> ten(10);
  fetch::fixed_point::FixedPoint<32, 32> m_ten(-10);
  fetch::fixed_point::FixedPoint<32, 32> e1 = fp64::Abs(one);
  fetch::fixed_point::FixedPoint<32, 32> e2 = fp64::Abs(m_one);
  fetch::fixed_point::FixedPoint<32, 32> e3 = fp64::Abs(one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e4 = fp64::Abs(m_one_point_five);
  fetch::fixed_point::FixedPoint<32, 32> e5 = fp64::Abs(ten);
  fetch::fixed_point::FixedPoint<32, 32> e6 = fp64::Abs(m_ten);

  EXPECT_EQ(e1, one);
  EXPECT_EQ(e2, one);
  EXPECT_EQ(e3, one_point_five);
  EXPECT_EQ(e4, one_point_five);
  EXPECT_EQ(e5, ten);
  EXPECT_EQ(e6, ten);
}