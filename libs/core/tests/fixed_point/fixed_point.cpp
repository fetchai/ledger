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
#include <limits>

TEST(FixedPointTest, Conversion)
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

TEST(FixedPointTest, Addition)
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

TEST(FixedPointTest, Subtraction)
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

TEST(FixedPointTest, Multiplication)
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

TEST(FixedPointTest, Division)
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

  try {
    EXPECT_EQ(int(two / zero), 0);
    EXPECT_EQ(float(two / zero), 0.0f);
    EXPECT_EQ(double(two / zero), 0.0);
    EXPECT_EQ(zero / zero, zero);
  } catch (std::exception &e) {
    std::cerr << "exception caught: " << e.what() << std::endl;
  }
}

TEST(FixedPointTest, Comparison)
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
}
