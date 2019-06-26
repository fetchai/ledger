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

#include "gtest/gtest.h"

#include <array>
#include <cmath>
#include <limits>

using namespace fetch::fixed_point;

TEST(FixedPointTest, Conversion_16_16)
{
  // Positive
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> two(2);

  EXPECT_EQ(static_cast<int>(one), 1);
  EXPECT_EQ(static_cast<int>(two), 2);
  EXPECT_EQ(static_cast<float>(one), 1.0f);
  EXPECT_EQ(static_cast<float>(two), 2.0f);
  EXPECT_EQ(static_cast<double>(one), 1.0);
  EXPECT_EQ(static_cast<double>(two), 2.0);

  // Negative
  FixedPoint<16, 16> m_one(-1);
  FixedPoint<16, 16> m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one), -1);
  EXPECT_EQ(static_cast<int>(m_two), -2);
  EXPECT_EQ(static_cast<float>(m_one), -1.0f);
  EXPECT_EQ(static_cast<float>(m_two), -2.0f);
  EXPECT_EQ(static_cast<double>(m_one), -1.0);
  EXPECT_EQ(static_cast<double>(m_two), -2.0);

  // _0
  FixedPoint<16, 16> zero(0);
  FixedPoint<16, 16> m_zero(-0);

  EXPECT_EQ(static_cast<int>(zero), 0);
  EXPECT_EQ(static_cast<int>(m_zero), 0);
  EXPECT_EQ(static_cast<float>(zero), 0.0f);
  EXPECT_EQ(static_cast<float>(m_zero), 0.0f);
  EXPECT_EQ(static_cast<double>(zero), 0.0);
  EXPECT_EQ(static_cast<double>(m_zero), 0.0);

  // Get raw value
  FixedPoint<16, 16> zero_point_five(0.5);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> two_point_five(2.5);
  FixedPoint<16, 16> m_one_point_five(-1.5);

  EXPECT_EQ(zero_point_five.Data(), 0x08000);
  EXPECT_EQ(one.Data(), 0x10000);
  EXPECT_EQ(one_point_five.Data(), 0x18000);
  EXPECT_EQ(two_point_five.Data(), 0x28000);

  // Convert from raw value
  FixedPoint<16, 16> two_point_five_raw(2, 0x08000);
  FixedPoint<16, 16> m_two_point_five_raw(-2, 0x08000);
  EXPECT_EQ(two_point_five, two_point_five_raw);
  EXPECT_EQ(m_one_point_five, m_two_point_five_raw);

  // Extreme cases:
  // smallest possible double representable to a FixedPoint
  FixedPoint<16, 16> infinitesimal(0.00002);
  // Largest fractional closest to one, representable to a FixedPoint
  FixedPoint<16, 16> almost_one(0.99999);
  // Largest fractional closest to one, representable to a FixedPoint
  FixedPoint<16, 16> largest_int(std::numeric_limits<int16_t>::max());

  // Smallest possible integer, increase by one, in order to allow for the fractional part.
  FixedPoint<16, 16> smallest_int(std::numeric_limits<int16_t>::min());

  // Largest possible Fixed Point number.
  FixedPoint<16, 16> largest_fixed_point = largest_int + almost_one;

  // Smallest possible Fixed Point number.
  FixedPoint<16, 16> smallest_fixed_point = smallest_int + almost_one;

  EXPECT_EQ(infinitesimal.Data(), fp32_t::SMALLEST_FRACTION);
  EXPECT_EQ(almost_one.Data(), fp32_t::LARGEST_FRACTION);
  EXPECT_EQ(largest_int.Data(), fp32_t::MAX_INT);
  EXPECT_EQ(smallest_int.Data(), fp32_t::MIN_INT);
  EXPECT_EQ(largest_fixed_point.Data(), fp32_t::MAX);
  EXPECT_EQ(smallest_fixed_point.Data(), fp32_t::MIN);

  EXPECT_EQ(fp32_t::MIN, 0x8000ffff);
  EXPECT_EQ(fp32_t::MAX, 0x7fffffff);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int32_t>::min());
  // On the other hand we expect to be exactly the same as the largest positive integer of int64_t
  EXPECT_TRUE(largest_fixed_point.Data() == std::numeric_limits<int32_t>::max());

  EXPECT_EQ(sizeof(one), 4);

  EXPECT_EQ(static_cast<int>(one), 1);
  EXPECT_EQ(static_cast<unsigned>(one), 1);
  EXPECT_EQ(static_cast<int32_t>(one), 1);
  EXPECT_EQ(static_cast<uint32_t>(one), 1);
  EXPECT_EQ(static_cast<long>(one), 1);
  EXPECT_EQ(static_cast<unsigned long>(one), 1);
  EXPECT_EQ(static_cast<int64_t>(one), 1);
  EXPECT_EQ(static_cast<uint64_t>(one), 1);

  EXPECT_EQ(fp32_t::TOLERANCE.Data(), 0x15);
  EXPECT_EQ(fp32_t::DECIMAL_DIGITS, 4);
}

TEST(FixedPointTest, Conversion_32_32)
{
  // Positive
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> two(2);

  EXPECT_EQ(static_cast<int>(one), 1);
  EXPECT_EQ(static_cast<int>(two), 2);
  EXPECT_EQ(static_cast<float>(one), 1.0f);
  EXPECT_EQ(static_cast<float>(two), 2.0f);
  EXPECT_EQ(static_cast<double>(one), 1.0);
  EXPECT_EQ(static_cast<double>(two), 2.0);

  // Negative
  FixedPoint<32, 32> m_one(-1);
  FixedPoint<32, 32> m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one), -1);
  EXPECT_EQ(static_cast<int>(m_two), -2);
  EXPECT_EQ(static_cast<float>(m_one), -1.0f);
  EXPECT_EQ(static_cast<float>(m_two), -2.0f);
  EXPECT_EQ(static_cast<double>(m_one), -1.0);
  EXPECT_EQ(static_cast<double>(m_two), -2.0);

  // _0
  FixedPoint<32, 32> zero(0);
  FixedPoint<32, 32> m_zero(-0);

  EXPECT_EQ(static_cast<int>(zero), 0);
  EXPECT_EQ(static_cast<int>(m_zero), 0);
  EXPECT_EQ(static_cast<float>(zero), 0.0f);
  EXPECT_EQ(static_cast<float>(m_zero), 0.0f);
  EXPECT_EQ(static_cast<double>(zero), 0.0);
  EXPECT_EQ(static_cast<double>(m_zero), 0.0);

  // Get raw value
  FixedPoint<32, 32> zero_point_five(0.5);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> two_point_five(2.5);
  FixedPoint<32, 32> m_one_point_five(-1.5);

  EXPECT_EQ(zero_point_five.Data(), 0x080000000);
  EXPECT_EQ(one.Data(), 0x100000000);
  EXPECT_EQ(one_point_five.Data(), 0x180000000);
  EXPECT_EQ(two_point_five.Data(), 0x280000000);

  // Convert from raw value
  FixedPoint<32, 32> two_point_five_raw(2, 0x080000000);
  FixedPoint<32, 32> m_two_point_five_raw(-2, 0x080000000);
  EXPECT_EQ(two_point_five, two_point_five_raw);
  EXPECT_EQ(m_one_point_five, m_two_point_five_raw);

  // Extreme cases:
  // smallest possible double representable to a FixedPoint
  FixedPoint<32, 32> infinitesimal(0.0000000004);
  // Largest fractional closest to one, representable to a FixedPoint
  FixedPoint<32, 32> almost_one(0.9999999998);
  // Largest fractional closest to one, representable to a FixedPoint
  FixedPoint<32, 32> largest_int(std::numeric_limits<int32_t>::max());

  // Smallest possible integer, increase by one, in order to allow for the fractional part.
  FixedPoint<32, 32> smallest_int(std::numeric_limits<int32_t>::min());

  // Largest possible Fixed Point number.
  FixedPoint<32, 32> largest_fixed_point = largest_int + almost_one;

  // Smallest possible Fixed Point number.
  FixedPoint<32, 32> smallest_fixed_point = smallest_int + almost_one;

  EXPECT_EQ(infinitesimal.Data(), fp64_t::SMALLEST_FRACTION);
  EXPECT_EQ(almost_one.Data(), fp64_t::LARGEST_FRACTION);
  EXPECT_EQ(largest_int.Data(), fp64_t::MAX_INT);
  EXPECT_EQ(smallest_int.Data(), fp64_t::MIN_INT);
  EXPECT_EQ(largest_fixed_point.Data(), fp64_t::MAX);
  EXPECT_EQ(smallest_fixed_point.Data(), fp64_t::MIN);
  EXPECT_EQ(fp64_t::MIN, 0x80000000ffffffff);
  EXPECT_EQ(fp64_t::MAX, 0x7fffffffffffffff);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int64_t>::min());
  // On the other hand we expect to be exactly the same as the largest positive integer of int64_t
  EXPECT_TRUE(largest_fixed_point.Data() == std::numeric_limits<int64_t>::max());

  EXPECT_EQ(sizeof(one), 8);

  EXPECT_EQ(static_cast<int>(one), 1);
  EXPECT_EQ(static_cast<unsigned>(one), 1);
  EXPECT_EQ(static_cast<int32_t>(one), 1);
  EXPECT_EQ(static_cast<uint32_t>(one), 1);
  EXPECT_EQ(static_cast<long>(one), 1);
  EXPECT_EQ(static_cast<uint64_t>(one), 1);
  EXPECT_EQ(static_cast<int64_t>(one), 1);
  EXPECT_EQ(static_cast<uint64_t>(one), 1);

  EXPECT_EQ(fp64_t::TOLERANCE.Data(), 0x200);
  EXPECT_EQ(fp64_t::DECIMAL_DIGITS, 9);
}

TEST(FixedPointTest, Addition_16_16)
{
  // Positive
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> two(2);

  EXPECT_EQ(static_cast<int>(one + two), 3);
  EXPECT_EQ(static_cast<float>(one + two), 3.0f);
  EXPECT_EQ(static_cast<double>(one + two), 3.0);

  // Negative
  FixedPoint<16, 16> m_one(-1);
  FixedPoint<16, 16> m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one + one), 0);
  EXPECT_EQ(static_cast<int>(m_one + m_two), -3);
  EXPECT_EQ(static_cast<float>(m_one + one), 0.0f);
  EXPECT_EQ(static_cast<float>(m_one + m_two), -3.0f);
  EXPECT_EQ(static_cast<double>(m_one + one), 0.0);
  EXPECT_EQ(static_cast<double>(m_one + m_two), -3.0);

  FixedPoint<16, 16> another{one};
  ++another;
  EXPECT_EQ(another, two);

  // _0
  FixedPoint<16, 16> zero(0);
  FixedPoint<16, 16> m_zero(-0);

  EXPECT_EQ(static_cast<int>(zero), 0);
  EXPECT_EQ(static_cast<int>(m_zero), 0);
  EXPECT_EQ(static_cast<float>(zero), 0.0f);
  EXPECT_EQ(static_cast<float>(m_zero), 0.0f);
  EXPECT_EQ(static_cast<double>(zero), 0.0);
  EXPECT_EQ(static_cast<double>(m_zero), 0.0);

  // Infinitesimal additions
  FixedPoint<16, 16> almost_one(0, fp64_t::LARGEST_FRACTION);
  FixedPoint<16, 16> infinitesimal(0, fp64_t::SMALLEST_FRACTION);

  // Largest possible fraction and smallest possible fraction should make us the value of 1
  EXPECT_EQ(almost_one + infinitesimal, one);
  // The same for negative
  EXPECT_EQ(-almost_one - infinitesimal, m_one);
}

TEST(FixedPointTest, Addition_32_32)
{
  // Positive
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> two(2);

  EXPECT_EQ(static_cast<int>(one + two), 3);
  EXPECT_EQ(static_cast<float>(one + two), 3.0f);
  EXPECT_EQ(static_cast<double>(one + two), 3.0);

  // Negative
  FixedPoint<32, 32> m_one(-1);
  FixedPoint<32, 32> m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one + one), 0);
  EXPECT_EQ(static_cast<int>(m_one + m_two), -3);
  EXPECT_EQ(static_cast<float>(m_one + one), 0.0f);
  EXPECT_EQ(static_cast<float>(m_one + m_two), -3.0f);
  EXPECT_EQ(static_cast<double>(m_one + one), 0.0);
  EXPECT_EQ(static_cast<double>(m_one + m_two), -3.0);

  // _0
  FixedPoint<32, 32> zero(0);
  FixedPoint<32, 32> m_zero(-0);

  EXPECT_EQ(static_cast<int>(zero), 0);
  EXPECT_EQ(static_cast<int>(m_zero), 0);
  EXPECT_EQ(static_cast<float>(zero), 0.0f);
  EXPECT_EQ(static_cast<float>(m_zero), 0.0f);
  EXPECT_EQ(static_cast<double>(zero), 0.0);
  EXPECT_EQ(static_cast<double>(m_zero), 0.0);

  // Infinitesimal additions
  FixedPoint<32, 32> almost_one(0, fp64_t::LARGEST_FRACTION);
  FixedPoint<32, 32> infinitesimal(0, fp64_t::SMALLEST_FRACTION);

  // Largest possible fraction and smallest possible fraction should make us the value of 1
  EXPECT_EQ(almost_one + infinitesimal, one);
  // The same for negative
  EXPECT_EQ(-almost_one - infinitesimal, m_one);
}

TEST(FixedPointTest, Subtraction_16_16)
{
  // Positive
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> two(2);

  EXPECT_EQ(static_cast<int>(two - one), 1);
  EXPECT_EQ(static_cast<float>(two - one), 1.0f);
  EXPECT_EQ(static_cast<double>(two - one), 1.0);

  EXPECT_EQ(static_cast<int>(one - two), -1);
  EXPECT_EQ(static_cast<float>(one - two), -1.0f);
  EXPECT_EQ(static_cast<double>(one - two), -1.0);

  // Negative
  FixedPoint<16, 16> m_one(-1);
  FixedPoint<16, 16> m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one - one), -2);
  EXPECT_EQ(static_cast<int>(m_one - m_two), 1);
  EXPECT_EQ(static_cast<float>(m_one - one), -2.0f);
  EXPECT_EQ(static_cast<float>(m_one - m_two), 1.0f);
  EXPECT_EQ(static_cast<double>(m_one - one), -2.0);
  EXPECT_EQ(static_cast<double>(m_one - m_two), 1.0);

  // Fractions
  FixedPoint<16, 16> almost_three(2, fp32_t::LARGEST_FRACTION);
  FixedPoint<16, 16> almost_two(1, fp32_t::LARGEST_FRACTION);

  EXPECT_EQ(almost_three - almost_two, one);
}

TEST(FixedPointTest, Subtraction_32_32)
{
  // Positive
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> two(2);

  EXPECT_EQ(static_cast<int>(two - one), 1);
  EXPECT_EQ(static_cast<float>(two - one), 1.0f);
  EXPECT_EQ(static_cast<double>(two - one), 1.0);

  EXPECT_EQ(static_cast<int>(one - two), -1);
  EXPECT_EQ(static_cast<float>(one - two), -1.0f);
  EXPECT_EQ(static_cast<double>(one - two), -1.0);

  // Negative
  FixedPoint<32, 32> m_one(-1);
  FixedPoint<32, 32> m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one - one), -2);
  EXPECT_EQ(static_cast<int>(m_one - m_two), 1);
  EXPECT_EQ(static_cast<float>(m_one - one), -2.0f);
  EXPECT_EQ(static_cast<float>(m_one - m_two), 1.0f);
  EXPECT_EQ(static_cast<double>(m_one - one), -2.0);
  EXPECT_EQ(static_cast<double>(m_one - m_two), 1.0);

  // Fractions
  FixedPoint<32, 32> almost_three(2, fp64_t::LARGEST_FRACTION);
  FixedPoint<32, 32> almost_two(1, fp64_t::LARGEST_FRACTION);

  EXPECT_EQ(almost_three - almost_two, one);
}

TEST(FixedPointTest, Multiplication_16_16)
{
  // Positive
  FixedPoint<16, 16> zero(0);
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> two(2);
  FixedPoint<16, 16> three(3);
  FixedPoint<16, 16> m_one(-1);

  EXPECT_EQ(two * one, two);
  EXPECT_EQ(one * 2, 2);
  EXPECT_EQ(m_one * zero, zero);
  EXPECT_EQ(m_one * one, m_one);
  EXPECT_EQ(static_cast<float>(two * 2.0f), 4.0f);
  EXPECT_EQ(static_cast<double>(three * 2.0), 6.0);

  EXPECT_EQ(static_cast<int>(one * two), 2);
  EXPECT_EQ(static_cast<float>(one * two), 2.0f);
  EXPECT_EQ(static_cast<double>(one * two), 2.0);

  EXPECT_EQ(static_cast<int>(two * zero), 0);
  EXPECT_EQ(static_cast<float>(two * zero), 0.0f);
  EXPECT_EQ(static_cast<double>(two * zero), 0.0);

  // Extreme cases
  FixedPoint<16, 16> almost_one(0, fp64_t::LARGEST_FRACTION);
  FixedPoint<16, 16> infinitesimal(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<16, 16> huge(0x4000, 0);
  FixedPoint<16, 16> small(0, 0x4000);

  EXPECT_EQ(almost_one * almost_one, almost_one - infinitesimal);
  EXPECT_EQ(almost_one * infinitesimal, zero);
  EXPECT_EQ(huge * infinitesimal, small);

  // (One of) largest possible multiplications
}

TEST(FixedPointTest, Multiplication_32_32)
{
  // Positive
  FixedPoint<32, 32> zero(0);
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> two(2);
  FixedPoint<32, 32> three(3);
  FixedPoint<32, 32> m_one(-1);

  EXPECT_EQ(two * one, two);
  EXPECT_EQ(one * 2, 2);
  EXPECT_EQ(m_one * zero, zero);
  EXPECT_EQ(m_one * one, m_one);
  EXPECT_EQ(static_cast<float>(two * 2.0f), 4.0f);
  EXPECT_EQ(static_cast<double>(three * 2.0), 6.0);

  EXPECT_EQ(static_cast<int>(one * two), 2);
  EXPECT_EQ(static_cast<float>(one * two), 2.0f);
  EXPECT_EQ(static_cast<double>(one * two), 2.0);

  EXPECT_EQ(static_cast<int>(two * zero), 0);
  EXPECT_EQ(static_cast<float>(two * zero), 0.0f);
  EXPECT_EQ(static_cast<double>(two * zero), 0.0);

  // Extreme cases
  FixedPoint<32, 32> almost_one(0, fp64_t::LARGEST_FRACTION);
  FixedPoint<32, 32> infinitesimal(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> huge(0x40000000, 0);
  FixedPoint<32, 32> small(0, 0x40000000);

  EXPECT_EQ(almost_one * almost_one, almost_one - infinitesimal);
  EXPECT_EQ(almost_one * infinitesimal, zero);
  EXPECT_EQ(huge * infinitesimal, small);
}

TEST(FixedPointTest, Division_16_16)
{
  // Positive
  FixedPoint<16, 16> zero(0);
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> two(2);

  EXPECT_EQ(static_cast<int>(two / one), 2);
  EXPECT_EQ(static_cast<float>(two / one), 2.0f);
  EXPECT_EQ(static_cast<double>(two / one), 2.0);

  EXPECT_EQ(static_cast<int>(one / two), 0);
  EXPECT_EQ(static_cast<float>(one / two), 0.5f);
  EXPECT_EQ(static_cast<double>(one / two), 0.5);

  // Extreme cases
  FixedPoint<16, 16> infinitesimal(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<16, 16> huge(0x4000, 0);
  FixedPoint<16, 16> small(0, 0x4000);

  EXPECT_EQ(small / infinitesimal, huge);
  EXPECT_EQ(infinitesimal / one, infinitesimal);
  EXPECT_EQ(one / huge, infinitesimal * 4);
  EXPECT_EQ(huge / infinitesimal, zero);

  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(two / zero));
  EXPECT_TRUE(fp32_t::isStateDivisionByZero());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(zero / zero));
  EXPECT_TRUE(fp32_t::isStateNaN());
}

TEST(FixedPointTest, Division_32_32)
{
  // Positive
  FixedPoint<32, 32> zero(0);
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> two(2);

  EXPECT_EQ(static_cast<int>(two / one), 2);
  EXPECT_EQ(static_cast<float>(two / one), 2.0f);
  EXPECT_EQ(static_cast<double>(two / one), 2.0);

  EXPECT_EQ(static_cast<int>(one / two), 0);
  EXPECT_EQ(static_cast<float>(one / two), 0.5f);
  EXPECT_EQ(static_cast<double>(one / two), 0.5);

  // Extreme cases
  FixedPoint<32, 32> infinitesimal(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> huge(0x40000000, 0);
  FixedPoint<32, 32> small(0, 0x40000000);

  EXPECT_EQ(small / infinitesimal, huge);
  EXPECT_EQ(infinitesimal / one, infinitesimal);
  EXPECT_EQ(one / huge, infinitesimal * 4);
  EXPECT_EQ(huge / infinitesimal, zero);

  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(two / zero));
  EXPECT_TRUE(fp64_t::isStateDivisionByZero());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(zero / zero));
  EXPECT_TRUE(fp64_t::isStateNaN());
}

TEST(FixedPointTest, Comparison_16_16)
{
  FixedPoint<16, 16> zero(0);
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> two(2);

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

  FixedPoint<16, 16> zero_point_five(0.5);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> two_point_five(2.5);

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

  FixedPoint<16, 16> m_zero(-0);
  FixedPoint<16, 16> m_one(-1.0);
  FixedPoint<16, 16> m_two(-2);

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

  EXPECT_TRUE(fp32_t::Constants.E == 2.718281828459045235360287471352662498);
  EXPECT_TRUE(fp32_t::Constants.LOG2E == 1.442695040888963407359924681001892137);
  EXPECT_TRUE(fp32_t::Constants.LOG10E == 0.434294481903251827651128918916605082);
  EXPECT_TRUE(fp32_t::Constants.LN2 == 0.693147180559945309417232121458176568);
  EXPECT_TRUE(fp32_t::Constants.LN10 == 2.302585092994045684017991454684364208);
  EXPECT_TRUE(fp32_t::Constants.PI == 3.141592653589793238462643383279502884);
  EXPECT_TRUE(fp32_t::Constants.PI_2 == 1.570796326794896619231321691639751442);
  EXPECT_TRUE(fp32_t::Constants.PI_4 == 0.785398163397448309615660845819875721);
  EXPECT_TRUE(fp32_t::Constants.INV_PI == 0.318309886183790671537767526745028724);
  EXPECT_TRUE(fp32_t::Constants.TWO_INV_PI == 0.636619772367581343075535053490057448);
  EXPECT_TRUE(fp32_t::Constants.TWO_INV_SQRTPI == 1.128379167095512573896158903121545172);
  EXPECT_TRUE(fp32_t::Constants.SQRT2 == 1.414213562373095048801688724209698079);
  EXPECT_TRUE(fp32_t::Constants.INV_SQRT2 == 0.707106781186547524400844362104849039);

  EXPECT_EQ(fp32_t::MAX_INT, 0x7fff0000);
  EXPECT_EQ(fp32_t::MIN_INT, 0x80000000);
  EXPECT_EQ(fp32_t::MAX, 0x7fffffff);
  EXPECT_EQ(fp32_t::MIN, 0x8000ffff);
  EXPECT_EQ(fp32_t::Constants.MAX_EXP.Data(), 0x000a65b9);
  EXPECT_EQ(fp32_t::Constants.MIN_EXP.Data(), 0xfff59a47);
}

TEST(FixedPointTest, Comparison_32_32)
{
  FixedPoint<32, 32> zero(0);
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> two(2);

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

  FixedPoint<32, 32> zero_point_five(0.5);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> two_point_five(2.5);

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

  FixedPoint<32, 32> m_zero(-0);
  FixedPoint<32, 32> m_one(-1.0);
  FixedPoint<32, 32> m_two(-2);

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

  EXPECT_TRUE(fp64_t::Constants.E == 2.718281828459045235360287471352662498);
  EXPECT_TRUE(fp64_t::Constants.LOG2E == 1.442695040888963407359924681001892137);
  EXPECT_TRUE(fp64_t::Constants.LOG10E == 0.434294481903251827651128918916605082);
  EXPECT_TRUE(fp64_t::Constants.LN2 == 0.693147180559945309417232121458176568);
  EXPECT_TRUE(fp64_t::Constants.LN10 == 2.302585092994045684017991454684364208);
  EXPECT_TRUE(fp64_t::Constants.PI == 3.141592653589793238462643383279502884);
  EXPECT_TRUE(fp64_t::Constants.PI / 2 == fp64_t::Constants.PI_2);
  EXPECT_TRUE(fp64_t::Constants.PI_4 == 0.785398163397448309615660845819875721);
  EXPECT_TRUE(one / fp64_t::Constants.PI == fp64_t::Constants.INV_PI);
  EXPECT_TRUE(fp64_t::Constants.TWO_INV_PI == 0.636619772367581343075535053490057448);
  EXPECT_TRUE(fp64_t::Constants.TWO_INV_SQRTPI == 1.128379167095512573896158903121545172);
  EXPECT_TRUE(fp64_t::Constants.SQRT2 == 1.414213562373095048801688724209698079);
  EXPECT_TRUE(fp64_t::Constants.INV_SQRT2 == 0.707106781186547524400844362104849039);

  EXPECT_EQ(fp64_t::MAX_INT, 0x7fffffff00000000);
  EXPECT_EQ(fp64_t::MIN_INT, 0x8000000000000000);
  EXPECT_EQ(fp64_t::MAX, 0x7fffffffffffffff);
  EXPECT_EQ(fp64_t::MIN, 0x80000000ffffffff);
  EXPECT_EQ(fp64_t::Constants.MAX_EXP.Data(), 0x000000157cd0e714);
  EXPECT_EQ(fp64_t::Constants.MIN_EXP.Data(), 0xffffffea832f18ec);
}

TEST(FixedPointTest, Exponential_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> two(2);
  FixedPoint<16, 16> negative{-0.40028143};
  FixedPoint<16, 16> e1    = fp32_t::Exp(one);
  FixedPoint<16, 16> e2    = fp32_t::Exp(two);
  FixedPoint<16, 16> e3    = fp32_t::Exp(negative);
  FixedPoint<16, 16> e_max = fp32_t::Exp(fp32_t::Constants.MAX_EXP);

  EXPECT_NEAR(static_cast<double>(e1) / std::exp(1.0), 1.0, static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2) / std::exp(2.0), 1.0, static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR((static_cast<double>(e3) - std::exp(static_cast<double>(negative))) /
                  std::exp(static_cast<double>(negative)),
              0, static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e_max) / std::exp(static_cast<double>(fp32_t::Constants.MAX_EXP)),
              1.0, static_cast<double>(fp32_t::TOLERANCE));

  fp32_t::stateClear();
  EXPECT_EQ(fp32_t::Exp(fp32_t::Constants.MAX_EXP + 1), fp32_t::Constants.MAX);
  EXPECT_TRUE(fp32_t::isStateOverflow());

  fp32_t step{0.001};
  fp32_t x{-10.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp32_t e     = fp32_t::Exp(x);
    double r     = std::exp(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, 10 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Exp: max error = " << max_error << std::endl;
  // std::cout << "Exp: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Exponential_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> two(2);
  FixedPoint<32, 32> ten(10);
  FixedPoint<32, 32> small(0.0001);
  FixedPoint<32, 32> tiny(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> negative{-0.40028143};
  FixedPoint<32, 32> e1 = fp64_t::Exp(one);
  FixedPoint<32, 32> e2 = fp64_t::Exp(two);
  FixedPoint<32, 32> e3 = fp64_t::Exp(small);
  FixedPoint<32, 32> e4 = fp64_t::Exp(tiny);
  FixedPoint<32, 32> e5 = fp64_t::Exp(negative);
  FixedPoint<32, 32> e6 = fp64_t::Exp(ten);

  EXPECT_NEAR(static_cast<double>(e1) - std::exp(static_cast<double>(one)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2) - std::exp(static_cast<double>(two)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3) - std::exp(static_cast<double>(small)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e4) - std::exp(static_cast<double>(tiny)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e5) - std::exp(static_cast<double>(negative)), 0,
              static_cast<double>(fp64_t::TOLERANCE));

  // For bigger values check relative error
  EXPECT_NEAR((static_cast<double>(e6) - std::exp(static_cast<double>(ten))) /
                  std::exp(static_cast<double>(ten)),
              0, static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR((static_cast<double>(fp64_t::Exp(fp64_t::Constants.MAX_EXP)) -
               std::exp(static_cast<double>(fp64_t::Constants.MAX_EXP))) /
                  std::exp(static_cast<double>(fp64_t::Constants.MAX_EXP)),
              0, static_cast<double>(fp64_t::TOLERANCE));

  // Out of range
  fp64_t::stateClear();
  EXPECT_EQ(fp64_t::Exp(fp64_t::Constants.MAX_EXP + 1), fp64_t::Constants.MAX);
  EXPECT_TRUE(fp64_t::isStateOverflow());

  // Negative values
  EXPECT_NEAR(static_cast<double>(fp64_t::Exp(-one)) - std::exp(-static_cast<double>(one)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(fp64_t::Exp(-two)) - std::exp(-static_cast<double>(two)), 0,
              static_cast<double>(fp64_t::TOLERANCE));

  // This particular error produces more than 1e-6 error failing the test
  EXPECT_NEAR(static_cast<double>(fp64_t::Exp(-ten)) - std::exp(-static_cast<double>(ten)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  // The rest pass with fp64_t::TOLERANCE
  EXPECT_NEAR(static_cast<double>(fp64_t::Exp(-small)) - std::exp(-static_cast<double>(small)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(fp64_t::Exp(-tiny)) - std::exp(-static_cast<double>(tiny)), 0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(fp64_t::Exp(fp64_t::Constants.MIN_EXP)) -
                  std::exp(static_cast<double>(fp64_t::Constants.MIN_EXP)),
              0, static_cast<double>(fp64_t::TOLERANCE));

  fp64_t step{0.0001};
  fp64_t x{-10.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::Exp(x);
    double r     = std::exp(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, 10 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Exp: max error = " << max_error << std::endl;
  // std::cout << "Exp: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_16_16_positive_x)
{
  FixedPoint<16, 16> a{-1.6519711627625};
  FixedPoint<16, 16> two{2};
  FixedPoint<16, 16> three{3};
  FixedPoint<16, 16> b{1.8464393615723};
  FixedPoint<16, 16> e1 = fp32_t::Pow(a, two);
  FixedPoint<16, 16> e2 = fp32_t::Pow(a, three);
  FixedPoint<16, 16> e3 = fp32_t::Pow(two, b);

  EXPECT_NEAR(static_cast<double>(e1 / std::pow(-1.6519711627625, 2)), 1.0,
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2 / std::pow(-1.6519711627625, 3)), 1.0,
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3 / std::pow(2, 1.8464393615723)), 1.0,
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Pow(a, a)));

  fp32_t step{0.001};
  fp32_t x{0.001};
  fp32_t y{0.001};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 100.0; x += step)
  {
    for (; y < 10.5; y += step)
    {
      fp32_t e     = fp32_t::Pow(x, y);
      double r     = std::pow(static_cast<double>(x), static_cast<double>(y));
      double delta = std::abs(static_cast<double>(e - r));
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Pow: max error = " << max_error << std::endl;
  // std::cout << "Pow: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_16_16_negative_x)
{
  fp32_t step{0.01};
  fp32_t x{-10};
  fp32_t y{-4};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 10.0; x += step)
  {
    for (; y < 4; ++y)
    {
      fp32_t e     = fp32_t::Pow(x, y);
      double r     = std::pow(static_cast<double>(x), static_cast<double>(y));
      double delta = std::abs(static_cast<double>(e - r));
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Pow: max error = " << max_error << std::endl;
  // std::cout << "Pow: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_32_32_positive_x)
{
  FixedPoint<32, 32> a{-1.6519711627625};
  FixedPoint<32, 32> two{2};
  FixedPoint<32, 32> three{3};
  FixedPoint<32, 32> b{1.8464393615723};
  FixedPoint<32, 32> e1 = fp64_t::Pow(a, two);
  FixedPoint<32, 32> e2 = fp64_t::Pow(a, three);
  FixedPoint<32, 32> e3 = fp64_t::Pow(two, b);

  EXPECT_NEAR(static_cast<double>(e1 / std::pow(-1.6519711627625, 2)), 1.0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2 / std::pow(-1.6519711627625, 3)), 1.0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3 / std::pow(2, 1.8464393615723)), 1.0,
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Pow(a, a)));

  fp64_t step{0.0001};
  fp64_t x{0.0001};
  fp64_t y{0.001};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 100.0; x += step)
  {
    for (; y < 40.5; y += step)
    {
      fp64_t e     = fp64_t::Pow(x, y);
      double r     = std::pow(static_cast<double>(x), static_cast<double>(y));
      double delta = std::abs(static_cast<double>(e - r));
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Pow: max error = " << max_error << std::endl;
  // std::cout << "Pow: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Pow_32_32_negative_x)
{
  fp64_t step{0.01};
  fp64_t x{-10};
  fp64_t y{-9};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 10.0; x += step)
  {
    for (; y < 9; ++y)
    {
      fp64_t e     = fp64_t::Pow(x, y);
      double r     = std::pow(static_cast<double>(x), static_cast<double>(y));
      double delta = std::abs(static_cast<double>(e - r));
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Pow: max error = " << max_error << std::endl;
  // std::cout << "Pow: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Logarithm_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> ten(10);
  FixedPoint<16, 16> huge(10000);
  FixedPoint<16, 16> small(0.001);
  FixedPoint<16, 16> tiny(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<16, 16> e1 = fp32_t::Log2(one);
  FixedPoint<16, 16> e2 = fp32_t::Log2(one_point_five);
  FixedPoint<16, 16> e3 = fp32_t::Log2(ten);
  FixedPoint<16, 16> e4 = fp32_t::Log2(huge);
  FixedPoint<16, 16> e5 = fp32_t::Log2(small);
  FixedPoint<16, 16> e6 = fp32_t::Log2(tiny);

  EXPECT_NEAR(static_cast<double>(e1), std::log2(static_cast<double>(one)),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2), std::log2(static_cast<double>(one_point_five)),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3), std::log2(static_cast<double>(ten)),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e4), std::log2(static_cast<double>(huge)),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e5), std::log2(static_cast<double>(small)),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e6), std::log2(static_cast<double>(tiny)),
              static_cast<double>(fp32_t::TOLERANCE));

  fp32_t step{0.001};
  fp32_t x{tiny};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp32_t e     = fp32_t::Log(x);
    double r     = std::log(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Log: max error = " << max_error << std::endl;
  // std::cout << "Log: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Logarithm_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> ten(10);
  FixedPoint<32, 32> huge(1000000000);
  FixedPoint<32, 32> small(0.0001);
  FixedPoint<32, 32> tiny(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> e1 = fp64_t::Log2(one);
  FixedPoint<32, 32> e2 = fp64_t::Log2(one_point_five);
  FixedPoint<32, 32> e3 = fp64_t::Log2(ten);
  FixedPoint<32, 32> e4 = fp64_t::Log2(huge);
  FixedPoint<32, 32> e5 = fp64_t::Log2(small);
  FixedPoint<32, 32> e6 = fp64_t::Log2(tiny);

  EXPECT_NEAR(static_cast<double>(e1), std::log2(static_cast<double>(one)),
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2), std::log2(static_cast<double>(one_point_five)),
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3), std::log2(static_cast<double>(ten)),
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e4), std::log2(static_cast<double>(huge)),
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e5), std::log2(static_cast<double>(small)),
              static_cast<double>(fp64_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e6), std::log2(static_cast<double>(tiny)),
              static_cast<double>(fp64_t::TOLERANCE));

  fp64_t step{0.0001};
  fp64_t x{tiny};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::Log(x);
    double r     = std::log(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
    if (delta > tolerance)
    {
      // std::cout << "delta = " << delta << std::endl;
      // std::cout << "e = " << e << std::endl;
      // std::cout << "r = " << r << std::endl;
      // std::cout << "x = " << x << std::endl;
    }
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Log: max error = " << max_error << std::endl;
  // std::cout << "Log: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Abs_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> m_one(-1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> m_one_point_five(-1.5);
  FixedPoint<16, 16> ten(10);
  FixedPoint<16, 16> m_ten(-10);
  FixedPoint<16, 16> e1 = fp32_t::Abs(one);
  FixedPoint<16, 16> e2 = fp32_t::Abs(m_one);
  FixedPoint<16, 16> e3 = fp32_t::Abs(one_point_five);
  FixedPoint<16, 16> e4 = fp32_t::Abs(m_one_point_five);
  FixedPoint<16, 16> e5 = fp32_t::Abs(ten);
  FixedPoint<16, 16> e6 = fp32_t::Abs(m_ten);

  EXPECT_EQ(e1, one);
  EXPECT_EQ(e2, one);
  EXPECT_EQ(e3, one_point_five);
  EXPECT_EQ(e4, one_point_five);
  EXPECT_EQ(e5, ten);
  EXPECT_EQ(e6, ten);
}

TEST(FixedPointTest, Abs_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> m_one(-1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> m_one_point_five(-1.5);
  FixedPoint<32, 32> ten(10);
  FixedPoint<32, 32> m_ten(-10);
  FixedPoint<32, 32> huge(999999999.0);
  FixedPoint<32, 32> e1 = fp64_t::Abs(one);
  FixedPoint<32, 32> e2 = fp64_t::Abs(m_one);
  FixedPoint<32, 32> e3 = fp64_t::Abs(one_point_five);
  FixedPoint<32, 32> e4 = fp64_t::Abs(m_one_point_five);
  FixedPoint<32, 32> e5 = fp64_t::Abs(ten);
  FixedPoint<32, 32> e6 = fp64_t::Abs(m_ten);
  FixedPoint<32, 32> e7 = fp64_t::Abs(huge);

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
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> m_one(-1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> m_one_point_five(-1.5);
  FixedPoint<16, 16> ten(10);
  FixedPoint<16, 16> m_ten(-10);
  FixedPoint<16, 16> x{1.6519711627625};
  FixedPoint<16, 16> huge(1000);
  FixedPoint<16, 16> e1 = fp32_t::Remainder(ten, one);
  FixedPoint<16, 16> e2 = fp32_t::Remainder(ten, m_one);
  FixedPoint<16, 16> e3 = fp32_t::Remainder(ten, one_point_five);
  FixedPoint<16, 16> e4 = fp32_t::Remainder(ten, m_one_point_five);
  FixedPoint<16, 16> e5 = fp32_t::Remainder(ten, x);
  FixedPoint<16, 16> e6 = fp32_t::Remainder(m_ten, x);
  FixedPoint<16, 16> e7 = fp32_t::Remainder(huge, fp32_t::Constants.PI);

  EXPECT_EQ(e1, std::remainder(static_cast<double>(ten), static_cast<double>(one)));
  EXPECT_EQ(e2, std::remainder(static_cast<double>(ten), static_cast<double>(m_one)));
  EXPECT_EQ(e3, std::remainder(static_cast<double>(ten), static_cast<double>(one_point_five)));
  EXPECT_EQ(e4, std::remainder(static_cast<double>(ten), static_cast<double>(m_one_point_five)));
  EXPECT_EQ(e5, std::remainder(static_cast<double>(ten), static_cast<double>(x)));
  EXPECT_EQ(e6, std::remainder(static_cast<double>(m_ten), static_cast<double>(x)));
  EXPECT_EQ(e7,
            std::remainder(static_cast<double>(huge), static_cast<double>(fp32_t::Constants.PI)));
}

TEST(FixedPointTest, Remainder_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> m_one(-1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> m_one_point_five(-1.5);
  FixedPoint<32, 32> ten(10);
  FixedPoint<32, 32> m_ten(-10);
  FixedPoint<32, 32> x{1.6519711627625};
  FixedPoint<32, 32> huge(1000000000);
  FixedPoint<32, 32> e1 = fp64_t::Remainder(ten, one);
  FixedPoint<32, 32> e2 = fp64_t::Remainder(ten, m_one);
  FixedPoint<32, 32> e3 = fp64_t::Remainder(ten, one_point_five);
  FixedPoint<32, 32> e4 = fp64_t::Remainder(ten, m_one_point_five);
  FixedPoint<32, 32> e5 = fp64_t::Remainder(ten, x);
  FixedPoint<32, 32> e6 = fp64_t::Remainder(m_ten, x);
  FixedPoint<32, 32> e7 = fp64_t::Remainder(huge, x);

  EXPECT_EQ(e1, std::remainder(static_cast<double>(ten), static_cast<double>(one)));
  EXPECT_EQ(e2, std::remainder(static_cast<double>(ten), static_cast<double>(m_one)));
  EXPECT_EQ(e3, std::remainder(static_cast<double>(ten), static_cast<double>(one_point_five)));
  EXPECT_EQ(e4, std::remainder(static_cast<double>(ten), static_cast<double>(m_one_point_five)));
  EXPECT_EQ(e5, std::remainder(static_cast<double>(ten), static_cast<double>(x)));
  EXPECT_EQ(e6, std::remainder(static_cast<double>(m_ten), static_cast<double>(x)));
  EXPECT_EQ(e7, std::remainder(static_cast<double>(huge), static_cast<double>(x)));
}

TEST(FixedPointTest, Fmod_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> m_one(-1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> m_one_point_five(-1.5);
  FixedPoint<16, 16> ten(10);
  FixedPoint<16, 16> m_ten(-10);
  FixedPoint<16, 16> x{1.6519711627625};
  FixedPoint<16, 16> e1 = fp32_t::Fmod(ten, one);
  FixedPoint<16, 16> e2 = fp32_t::Fmod(ten, m_one);
  FixedPoint<16, 16> e3 = fp32_t::Fmod(ten, one_point_five);
  FixedPoint<16, 16> e4 = fp32_t::Fmod(ten, m_one_point_five);
  FixedPoint<16, 16> e5 = fp32_t::Fmod(ten, x);
  FixedPoint<16, 16> e6 = fp32_t::Fmod(m_ten, x);

  EXPECT_EQ(e1, std::fmod(static_cast<double>(ten), static_cast<double>(one)));
  EXPECT_EQ(e2, std::fmod(static_cast<double>(ten), static_cast<double>(m_one)));
  EXPECT_EQ(e3, std::fmod(static_cast<double>(ten), static_cast<double>(one_point_five)));
  EXPECT_EQ(e4, std::fmod(static_cast<double>(ten), static_cast<double>(m_one_point_five)));
  EXPECT_EQ(e5, std::fmod(static_cast<double>(ten), static_cast<double>(x)));
  EXPECT_EQ(e6, std::fmod(static_cast<double>(m_ten), static_cast<double>(x)));
}

TEST(FixedPointTest, Fmod_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> m_one(-1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> m_one_point_five(-1.5);
  FixedPoint<32, 32> ten(10);
  FixedPoint<32, 32> m_ten(-10);
  FixedPoint<32, 32> x{1.6519711627625};
  FixedPoint<32, 32> e1 = fp64_t::Fmod(ten, one);
  FixedPoint<32, 32> e2 = fp64_t::Fmod(ten, m_one);
  FixedPoint<32, 32> e3 = fp64_t::Fmod(ten, one_point_five);
  FixedPoint<32, 32> e4 = fp64_t::Fmod(ten, m_one_point_five);
  FixedPoint<32, 32> e5 = fp64_t::Fmod(ten, x);
  FixedPoint<32, 32> e6 = fp64_t::Fmod(m_ten, x);

  EXPECT_EQ(e1, std::fmod(static_cast<double>(ten), static_cast<double>(one)));
  EXPECT_EQ(e2, std::fmod(static_cast<double>(ten), static_cast<double>(m_one)));
  EXPECT_EQ(e3, std::fmod(static_cast<double>(ten), static_cast<double>(one_point_five)));
  EXPECT_EQ(e4, std::fmod(static_cast<double>(ten), static_cast<double>(m_one_point_five)));
  EXPECT_EQ(e5, std::fmod(static_cast<double>(ten), static_cast<double>(x)));
  EXPECT_EQ(e6, std::fmod(static_cast<double>(m_ten), static_cast<double>(x)));
}

TEST(FixedPointTest, SQRT_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> two(2);
  FixedPoint<16, 16> four(4);
  FixedPoint<16, 16> ten(10);
  FixedPoint<16, 16> huge(10000);
  FixedPoint<16, 16> small(0.0001);
  FixedPoint<16, 16> tiny(0, fp32_t::SMALLEST_FRACTION);
  FixedPoint<16, 16> e1 = fp32_t::Sqrt(one);
  FixedPoint<16, 16> e2 = fp32_t::Sqrt(one_point_five);
  FixedPoint<16, 16> e3 = fp32_t::Sqrt(two);
  FixedPoint<16, 16> e4 = fp32_t::Sqrt(four);
  FixedPoint<16, 16> e5 = fp32_t::Sqrt(ten);
  FixedPoint<16, 16> e6 = fp32_t::Sqrt(huge);
  FixedPoint<16, 16> e7 = fp32_t::Sqrt(small);
  FixedPoint<16, 16> e8 = fp32_t::Sqrt(tiny);

  double delta = static_cast<double>(e1) - std::sqrt(static_cast<double>(one));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(one)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::sqrt(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::sqrt(static_cast<double>(two));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(two)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e3 - fp32_t::Constants.SQRT2);
  EXPECT_NEAR(delta / static_cast<double>(fp32_t::Constants.SQRT2), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::sqrt(static_cast<double>(four));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(four)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e5) - std::sqrt(static_cast<double>(ten));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(ten)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::sqrt(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(huge)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::sqrt(static_cast<double>(small));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(small)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::sqrt(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));

  // Sqrt of a negative
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Sqrt(-one)));

  fp32_t step{0.01};
  fp32_t x{tiny}, max{huge};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 4 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < max; x += step)
  {
    fp32_t e     = fp32_t::Sqrt(x);
    double r     = std::sqrt(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, 5 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Sqrt: max error = " << max_error << std::endl;
  // std::cout << "Sqrt: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, SQRT_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> two(2);
  FixedPoint<32, 32> four(4);
  FixedPoint<32, 32> ten(10);
  FixedPoint<32, 32> huge(1000000000);
  FixedPoint<32, 32> small(0.0001);
  FixedPoint<32, 32> tiny(0, fp64_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> e1 = fp64_t::Sqrt(one);
  FixedPoint<32, 32> e2 = fp64_t::Sqrt(one_point_five);
  FixedPoint<32, 32> e3 = fp64_t::Sqrt(two);
  FixedPoint<32, 32> e4 = fp64_t::Sqrt(four);
  FixedPoint<32, 32> e5 = fp64_t::Sqrt(ten);
  FixedPoint<32, 32> e6 = fp64_t::Sqrt(huge);
  FixedPoint<32, 32> e7 = fp64_t::Sqrt(small);
  FixedPoint<32, 32> e8 = fp64_t::Sqrt(tiny);

  double delta = static_cast<double>(e1) - std::sqrt(static_cast<double>(one));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(one)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::sqrt(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::sqrt(static_cast<double>(two));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(two)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e3 - fp64_t::Constants.SQRT2);
  EXPECT_NEAR(delta / static_cast<double>(fp64_t::Constants.SQRT2), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::sqrt(static_cast<double>(four));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(four)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e5) - std::sqrt(static_cast<double>(ten));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(ten)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::sqrt(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(huge)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::sqrt(static_cast<double>(small));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(small)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::sqrt(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));

  // Sqrt of a negative
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Sqrt(-one)));

  fp64_t step{0.001};
  fp64_t x{tiny}, max{huge};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::Sqrt(x);
    double r     = std::sqrt(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, 10 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Sqrt: max error = " << max_error << std::endl;
  // std::cout << "Sqrt: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Sin_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> huge(2000);
  FixedPoint<16, 16> small(0.0001);
  FixedPoint<16, 16> tiny(0, fp32_t::SMALLEST_FRACTION);
  FixedPoint<16, 16> e1  = fp32_t::Sin(one);
  FixedPoint<16, 16> e2  = fp32_t::Sin(one_point_five);
  FixedPoint<16, 16> e3  = fp32_t::Sin(fp32_t::_0);
  FixedPoint<16, 16> e4  = fp32_t::Sin(huge);
  FixedPoint<16, 16> e5  = fp32_t::Sin(small);
  FixedPoint<16, 16> e6  = fp32_t::Sin(tiny);
  FixedPoint<16, 16> e7  = fp32_t::Sin(fp32_t::Constants.PI);
  FixedPoint<16, 16> e8  = fp32_t::Sin(-fp32_t::Constants.PI);
  FixedPoint<16, 16> e9  = fp32_t::Sin(fp32_t::Constants.PI * 2);
  FixedPoint<16, 16> e10 = fp32_t::Sin(fp32_t::Constants.PI * 4);
  FixedPoint<16, 16> e11 = fp32_t::Sin(fp32_t::Constants.PI * 100);
  FixedPoint<16, 16> e12 = fp32_t::Sin(fp32_t::Constants.PI_2);
  FixedPoint<16, 16> e13 = fp32_t::Sin(-fp32_t::Constants.PI_2);
  FixedPoint<16, 16> e14 = fp32_t::Sin(fp32_t::Constants.PI_4);
  FixedPoint<16, 16> e15 = fp32_t::Sin(-fp32_t::Constants.PI_4);
  FixedPoint<16, 16> e16 = fp32_t::Sin(fp32_t::Constants.PI_4 * 3);

  double delta = static_cast<double>(e1) - std::sin(static_cast<double>(one));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(one)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::sin(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::sin(static_cast<double>(fp32_t::_0));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::sin(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(huge)), 0.0,
              0.002);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e5) - std::sin(static_cast<double>(small));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(small)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::sin(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::sin(static_cast<double>(fp32_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::sin(static_cast<double>((-fp32_t::Constants.PI)));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e9) - std::sin(static_cast<double>((fp32_t::Constants.PI * 2)));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e10) - std::sin(static_cast<double>((fp32_t::Constants.PI * 4)));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e11) - std::sin(static_cast<double>((fp32_t::Constants.PI * 100)));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::sin(static_cast<double>((fp32_t::Constants.PI_2)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(fp32_t::Constants.PI_2)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e13) - std::sin(static_cast<double>((-fp32_t::Constants.PI_2)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(-fp32_t::Constants.PI_2)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e14) - std::sin(static_cast<double>((fp32_t::Constants.PI_4)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(fp32_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e15) - std::sin(static_cast<double>((-fp32_t::Constants.PI_4)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(-fp32_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e16) - std::sin(static_cast<double>((fp32_t::Constants.PI_4 * 3)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(fp32_t::Constants.PI_4 * 3)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));

  fp32_t step{0.1};
  fp32_t x{-fp32_t::Constants.PI * 10};
  for (; x < fp32_t::Constants.PI * 10; x += step)
  {
    fp32_t e = fp32_t::Sin(x);
    delta    = static_cast<double>(e) - std::sin(static_cast<double>(x));
    EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  }
}

TEST(FixedPointTest, Sin_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> huge(20000000);
  FixedPoint<32, 32> small(0.0000001);
  FixedPoint<32, 32> tiny(0, fp32_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> e1  = fp64_t::Sin(one);
  FixedPoint<32, 32> e2  = fp64_t::Sin(one_point_five);
  FixedPoint<32, 32> e3  = fp64_t::Sin(fp64_t::_0);
  FixedPoint<32, 32> e4  = fp64_t::Sin(huge);
  FixedPoint<32, 32> e5  = fp64_t::Sin(small);
  FixedPoint<32, 32> e6  = fp64_t::Sin(tiny);
  FixedPoint<32, 32> e7  = fp64_t::Sin(fp64_t::Constants.PI);
  FixedPoint<32, 32> e8  = fp64_t::Sin(-fp64_t::Constants.PI);
  FixedPoint<32, 32> e9  = fp64_t::Sin(fp64_t::Constants.PI * 2);
  FixedPoint<32, 32> e10 = fp64_t::Sin(fp64_t::Constants.PI * 4);
  FixedPoint<32, 32> e11 = fp64_t::Sin(fp64_t::Constants.PI * 100);
  FixedPoint<32, 32> e12 = fp64_t::Sin(fp64_t::Constants.PI_2);
  FixedPoint<32, 32> e13 = fp64_t::Sin(-fp64_t::Constants.PI_2);
  FixedPoint<32, 32> e14 = fp64_t::Sin(fp64_t::Constants.PI_4);
  FixedPoint<32, 32> e15 = fp64_t::Sin(-fp64_t::Constants.PI_4);
  FixedPoint<32, 32> e16 = fp64_t::Sin(fp64_t::Constants.PI_4 * 3);

  double delta = static_cast<double>(e1) - std::sin(static_cast<double>(one));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(one)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::sin(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::sin(static_cast<double>(fp64_t::_0));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::sin(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(huge)), 0.0,
              0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e5) - std::sin(static_cast<double>(small));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(small)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::sin(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::sin(static_cast<double>(fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::sin(static_cast<double>(-fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e9) - std::sin(static_cast<double>(fp64_t::Constants.PI * 2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e10) - std::sin(static_cast<double>(fp64_t::Constants.PI * 4));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e11) - std::sin(static_cast<double>(fp64_t::Constants.PI * 100));
  EXPECT_NEAR(delta, 0.0,
              static_cast<double>(fp64_t::TOLERANCE));  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::sin(static_cast<double>(fp64_t::Constants.PI_2));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(fp64_t::Constants.PI_2)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e13) - std::sin(static_cast<double>(-fp64_t::Constants.PI_2));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(-fp64_t::Constants.PI_2)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e14) - std::sin(static_cast<double>(fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e15) - std::sin(static_cast<double>(-fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(-fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e16) - std::sin(static_cast<double>(fp64_t::Constants.PI_4 * 3));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(fp64_t::Constants.PI_4 * 3)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));

  fp64_t step{0.001};
  fp64_t x{-fp64_t::Constants.PI * 100};
  for (; x < fp64_t::Constants.PI * 100; x += step)
  {
    fp64_t e = fp64_t::Sin(x);
    delta    = static_cast<double>(e) - std::sin(static_cast<double>(x));
    EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  }
}

TEST(FixedPointTest, Cos_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> huge(2000);
  FixedPoint<16, 16> small(0.0001);
  FixedPoint<16, 16> tiny(0, fp32_t::SMALLEST_FRACTION);
  FixedPoint<16, 16> e1  = fp32_t::Cos(one);
  FixedPoint<16, 16> e2  = fp32_t::Cos(one_point_five);
  FixedPoint<16, 16> e3  = fp32_t::Cos(fp32_t::_0);
  FixedPoint<16, 16> e4  = fp32_t::Cos(huge);
  FixedPoint<16, 16> e5  = fp32_t::Cos(small);
  FixedPoint<16, 16> e6  = fp32_t::Cos(tiny);
  FixedPoint<16, 16> e7  = fp32_t::Cos(fp32_t::Constants.PI);
  FixedPoint<16, 16> e8  = fp32_t::Cos(-fp32_t::Constants.PI);
  FixedPoint<16, 16> e9  = fp32_t::Cos(fp32_t::Constants.PI * 2);
  FixedPoint<16, 16> e10 = fp32_t::Cos(fp32_t::Constants.PI * 4);
  FixedPoint<16, 16> e11 = fp32_t::Cos(fp32_t::Constants.PI * 100);
  FixedPoint<16, 16> e12 = fp32_t::Cos(fp32_t::Constants.PI_2);
  FixedPoint<16, 16> e13 = fp32_t::Cos(-fp32_t::Constants.PI_2);
  FixedPoint<16, 16> e14 = fp32_t::Cos(fp32_t::Constants.PI_4);
  FixedPoint<16, 16> e15 = fp32_t::Cos(-fp32_t::Constants.PI_4);
  FixedPoint<16, 16> e16 = fp32_t::Cos(fp32_t::Constants.PI_4 * 3);

  double delta = static_cast<double>(e1) - std::cos(static_cast<double>(one));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(one)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::cos(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::cos(static_cast<double>(fp32_t::_0));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(fp32_t::_0)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::cos(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(huge)), 0.0,
              0.012);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e5) - std::cos(static_cast<double>(small));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(small)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::cos(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::cos(static_cast<double>(fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::cos(static_cast<double>(-fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e9) - std::cos(static_cast<double>(fp64_t::Constants.PI * 2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e10) - std::cos(static_cast<double>(fp64_t::Constants.PI * 4));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e11) - std::cos(static_cast<double>(fp64_t::Constants.PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::cos(static_cast<double>(fp64_t::Constants.PI_2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e13) - std::cos(static_cast<double>(-fp64_t::Constants.PI_2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e14) - std::cos(static_cast<double>(fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e15) - std::cos(static_cast<double>(-fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(-fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e16) - std::cos(static_cast<double>(fp64_t::Constants.PI_4 * 3));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(fp64_t::Constants.PI_4 * 3)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));

  fp32_t step{0.1};
  fp32_t x{-fp32_t::Constants.PI * 10};
  for (; x < fp32_t::Constants.PI * 10; x += step)
  {
    fp32_t e = fp32_t::Cos(x);
    delta    = static_cast<double>(e) - std::cos(static_cast<double>(x));
    EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  }
}

TEST(FixedPointTest, Cos_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> huge(2000);
  FixedPoint<32, 32> small(0.0001);
  FixedPoint<32, 32> tiny(0, fp32_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> e1  = fp64_t::Cos(one);
  FixedPoint<32, 32> e2  = fp64_t::Cos(one_point_five);
  FixedPoint<32, 32> e3  = fp64_t::Cos(fp64_t::_0);
  FixedPoint<32, 32> e4  = fp64_t::Cos(huge);
  FixedPoint<32, 32> e5  = fp64_t::Cos(small);
  FixedPoint<32, 32> e6  = fp64_t::Cos(tiny);
  FixedPoint<32, 32> e7  = fp64_t::Cos(fp64_t::Constants.PI);
  FixedPoint<32, 32> e8  = fp64_t::Cos(-fp64_t::Constants.PI);
  FixedPoint<32, 32> e9  = fp64_t::Cos(fp64_t::Constants.PI * 2);
  FixedPoint<32, 32> e10 = fp64_t::Cos(fp64_t::Constants.PI * 4);
  FixedPoint<32, 32> e11 = fp64_t::Cos(fp64_t::Constants.PI * 100);
  FixedPoint<32, 32> e12 = fp64_t::Cos(fp64_t::Constants.PI_2);
  FixedPoint<32, 32> e13 = fp64_t::Cos(-fp64_t::Constants.PI_2);
  FixedPoint<32, 32> e14 = fp64_t::Cos(fp64_t::Constants.PI_4);
  FixedPoint<32, 32> e15 = fp64_t::Cos(-fp64_t::Constants.PI_4);
  FixedPoint<32, 32> e16 = fp64_t::Cos(fp64_t::Constants.PI_4 * 3);

  double delta = static_cast<double>(e1) - std::cos(static_cast<double>(one));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(one)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::cos(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::cos(static_cast<double>(fp64_t::_0));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(fp64_t::_0)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::cos(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(huge)), 0.0,
              0.002);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e5) - std::cos(static_cast<double>(small));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(small)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::cos(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::cos(static_cast<double>(fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::cos(static_cast<double>(-fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e9) - std::cos(static_cast<double>(fp64_t::Constants.PI * 2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e10) - std::cos(static_cast<double>(fp64_t::Constants.PI * 4));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e11) - std::cos(static_cast<double>(fp64_t::Constants.PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::cos(static_cast<double>(fp64_t::Constants.PI_2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e13) - std::cos(static_cast<double>(-fp64_t::Constants.PI_2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e14) - std::cos(static_cast<double>(fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e15) - std::cos(static_cast<double>(-fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(-fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e16) - std::cos(static_cast<double>(fp64_t::Constants.PI_4 * 3));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(fp64_t::Constants.PI_4 * 3)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));

  fp64_t step{0.1};
  fp64_t x{-fp64_t::Constants.PI * 100};
  for (; x < fp64_t::Constants.PI * 100; x += step)
  {
    fp64_t e = fp64_t::Cos(x);
    delta    = static_cast<double>(e) - std::cos(static_cast<double>(x));
    EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  }
}

TEST(FixedPointTest, Tan_16_16)
{
  FixedPoint<16, 16> one(1);
  FixedPoint<16, 16> one_point_five(1.5);
  FixedPoint<16, 16> huge(2000);
  FixedPoint<16, 16> small(0.0001);
  FixedPoint<16, 16> tiny(0, fp32_t::SMALLEST_FRACTION);
  FixedPoint<16, 16> e1  = fp32_t::Tan(one);
  FixedPoint<16, 16> e2  = fp32_t::Tan(one_point_five);
  FixedPoint<16, 16> e3  = fp32_t::Tan(fp32_t::_0);
  FixedPoint<16, 16> e4  = fp32_t::Tan(huge);
  FixedPoint<16, 16> e5  = fp32_t::Tan(small);
  FixedPoint<16, 16> e6  = fp32_t::Tan(tiny);
  FixedPoint<16, 16> e7  = fp32_t::Tan(fp32_t::Constants.PI);
  FixedPoint<16, 16> e8  = fp32_t::Tan(-fp32_t::Constants.PI);
  FixedPoint<16, 16> e9  = fp32_t::Tan(fp32_t::Constants.PI * 2);
  FixedPoint<16, 16> e10 = fp32_t::Tan(fp32_t::Constants.PI * 4);
  FixedPoint<16, 16> e11 = fp32_t::Tan(fp32_t::Constants.PI * 100);
  FixedPoint<16, 16> e12 = fp32_t::Tan(fp32_t::Constants.PI_4);
  FixedPoint<16, 16> e13 = fp32_t::Tan(-fp32_t::Constants.PI_4);
  FixedPoint<16, 16> e14 = fp32_t::Tan(fp32_t::Constants.PI_4 * 3);

  double delta = static_cast<double>(e1) - std::tan(static_cast<double>(one));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(one)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::tan(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::tan(static_cast<double>(fp32_t::_0));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::tan(static_cast<double>(huge));
  // Tan for larger arguments loses precision
  EXPECT_NEAR(delta / std::tan(static_cast<double>(huge)), 0.0, 0.012);
  delta = static_cast<double>(e5) - std::tan(static_cast<double>(small));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(small)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::tan(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::tan(static_cast<double>(fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::tan(static_cast<double>(-fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e9) - std::tan(static_cast<double>(fp64_t::Constants.PI * 2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e10) - std::tan(static_cast<double>(fp64_t::Constants.PI * 4));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e11) - std::tan(static_cast<double>(fp64_t::Constants.PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::tan(static_cast<double>(fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e13) - std::tan(static_cast<double>(-fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(-fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));
  delta = static_cast<double>(e14) - std::tan(static_cast<double>(fp64_t::Constants.PI_4 * 3));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(fp64_t::Constants.PI_4 * 3)), 0.0,
              static_cast<double>(fp32_t::TOLERANCE));

  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::Tan(fp32_t::Constants.PI_2)));
  EXPECT_TRUE(fp32_t::isNegInfinity(fp32_t::Tan(-fp32_t::Constants.PI_2)));

  fp32_t step{0.001}, offset{step * 10};
  fp32_t x{-fp32_t::Constants.PI_2}, max{fp32_t::Constants.PI_2};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < max; x += step)
  {
    fp32_t e     = fp32_t::Tan(x);
    double r     = std::tan(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  // EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Tan: max error = " << max_error << std::endl;
  // std::cout << "Tan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, Tan_32_32)
{
  FixedPoint<32, 32> one(1);
  FixedPoint<32, 32> one_point_five(1.5);
  FixedPoint<32, 32> huge(2000);
  FixedPoint<32, 32> tiny(0, fp32_t::SMALLEST_FRACTION);
  FixedPoint<32, 32> e1  = fp64_t::Tan(one);
  FixedPoint<32, 32> e2  = fp64_t::Tan(one_point_five);
  FixedPoint<32, 32> e3  = fp64_t::Tan(fp64_t::_0);
  FixedPoint<32, 32> e4  = fp64_t::Tan(huge);
  FixedPoint<32, 32> e5  = fp64_t::Tan(tiny);
  FixedPoint<32, 32> e6  = fp64_t::Tan(fp64_t::Constants.PI);
  FixedPoint<32, 32> e7  = fp64_t::Tan(-fp64_t::Constants.PI);
  FixedPoint<32, 32> e8  = fp64_t::Tan(fp64_t::Constants.PI * 2);
  FixedPoint<32, 32> e9  = fp64_t::Tan(fp64_t::Constants.PI * 4);
  FixedPoint<32, 32> e10 = fp64_t::Tan(fp64_t::Constants.PI * 100);
  FixedPoint<32, 32> e11 = fp64_t::Tan(fp64_t::Constants.PI_4);
  FixedPoint<32, 32> e12 = fp64_t::Tan(-fp64_t::Constants.PI_4);
  FixedPoint<32, 32> e13 = fp64_t::Tan(fp64_t::Constants.PI_4 * 3);

  double delta = static_cast<double>(e1) - std::tan(static_cast<double>(one));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(one)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e2) - std::tan(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e3) - std::tan(static_cast<double>(fp64_t::_0));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e4) - std::tan(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(huge)), 0.0, 0.012);
  delta = static_cast<double>(e5) - std::tan(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(tiny)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e6) - std::tan(static_cast<double>(fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e7) - std::tan(static_cast<double>(-fp64_t::Constants.PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e8) - std::tan(static_cast<double>(fp64_t::Constants.PI * 2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e9) - std::tan(static_cast<double>(fp64_t::Constants.PI * 4));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e10) - std::tan(static_cast<double>(fp64_t::Constants.PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e11) - std::tan(static_cast<double>(fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e12) - std::tan(static_cast<double>(-fp64_t::Constants.PI_4));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(-fp64_t::Constants.PI_4)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));
  delta = static_cast<double>(e13) - std::tan(static_cast<double>(fp64_t::Constants.PI_4 * 3));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(fp64_t::Constants.PI_4 * 3)), 0.0,
              static_cast<double>(fp64_t::TOLERANCE));

  // (843, private) Replace with IsInfinity()
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::Tan(fp64_t::Constants.PI_2)));
  EXPECT_TRUE(fp64_t::isNegInfinity(fp64_t::Tan(-fp64_t::Constants.PI_2)));

  fp64_t step{0.0001}, offset{step * 100};
  fp64_t x{-fp64_t::Constants.PI_2}, max{fp64_t::Constants.PI_2};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp64_t::TOLERANCE);
  for (; x < max; x += step)
  {
    fp64_t e     = fp64_t::Tan(x);
    double r     = std::tan(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  // EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "Tan: max error = " << max_error << std::endl;
  // std::cout << "Tan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASin_16_16)
{
  fp32_t step{0.001};
  fp32_t x{-0.99};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 1.0; x += step)
  {
    fp32_t e     = fp32_t::ASin(x);
    double r     = std::asin(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ASin: max error = " << max_error << std::endl;
  // std::cout << "ASin: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASin_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-0.999};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 1.0; x += step)
  {
    fp64_t e     = fp64_t::ASin(x);
    double r     = std::asin(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ASin: max error = " << max_error << std::endl;
  // std::cout << "ASin: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACos_16_16)
{
  fp32_t step{0.001};
  fp32_t x{-0.99};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 1.0; x += step)
  {
    fp32_t e     = fp32_t::ACos(x);
    double r     = std::acos(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ACos: max error = " << max_error << std::endl;
  // std::cout << "ACos: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACos_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-1.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 1.0; x += step)
  {
    fp64_t e     = fp64_t::ACos(x);
    double r     = std::acos(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ACos: max error = " << max_error << std::endl;
  // std::cout << "ACos: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan_16_16)
{
  fp32_t step{0.001};
  fp32_t x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp32_t e     = fp32_t::ATan(x);
    double r     = std::atan(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ATan: max error = " << max_error << std::endl;
  // std::cout << "ATan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::ATan(x);
    double r     = std::atan(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ATan: max error = " << max_error << std::endl;
  // std::cout << "ATan: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan2_16_16)
{
  fp32_t step{0.01};
  fp32_t x{-2.0};
  fp32_t y{-2.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 2.0; x += step)
  {
    for (; y < 2.0; y += step)
    {
      fp32_t e     = fp32_t::ATan2(y, x);
      double r     = std::atan2(static_cast<double>(y), static_cast<double>(x));
      double delta = std::abs(static_cast<double>(e - r));
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ATan2: max error = " << max_error << std::endl;
  // std::cout << "ATan2: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATan2_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-2.0};
  fp64_t y{-2.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 2.0; x += step)
  {
    for (; y < 2.0; y += step)
    {
      fp64_t e     = fp64_t::ATan2(y, x);
      double r     = std::atan2(static_cast<double>(y), static_cast<double>(x));
      double delta = std::abs(static_cast<double>(e - r));
      max_error    = std::max(max_error, delta);
      avg_error += delta;
      iterations++;
    }
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ATan2: max error = " << max_error << std::endl;
  // std::cout << "ATan2: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, SinH_16_16)
{
  fp32_t step{0.001};
  fp32_t x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2.0 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 3.0; x += step)
  {
    fp32_t e     = fp32_t::SinH(x);
    double r     = std::sinh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "SinH: max error = " << max_error << std::endl;
  // std::cout << "SinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, SinH_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::SinH(x);
    double r     = std::sinh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "SinH: max error = " << max_error << std::endl;
  // std::cout << "SinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, CosH_16_16)
{
  fp32_t step{0.001};
  fp32_t x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2.0 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 3.0; x += step)
  {
    fp32_t e     = fp32_t::CosH(x);
    double r     = std::cosh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "CosH: max error = " << max_error << std::endl;
  // std::cout << "CosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, CosH_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::CosH(x);
    double r     = std::cosh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "CosH: max error = " << max_error << std::endl;
  // std::cout << "CosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, TanH_16_16)
{
  fp32_t step{0.001};
  fp32_t x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 3.0; x += step)
  {
    fp32_t e     = fp32_t::TanH(x);
    double r     = std::tanh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "TanH: max error = " << max_error << std::endl;
  // std::cout << "TanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, TanH_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::TanH(x);
    double r     = std::tanh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "TanH: max error = " << max_error << std::endl;
  // std::cout << "TanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASinH_16_16)
{
  fp32_t step{0.001};
  fp32_t x{-3.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 3.0; x += step)
  {
    fp32_t e     = fp32_t::ASinH(x);
    double r     = std::asinh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ASinH: max error = " << max_error << std::endl;
  // std::cout << "ASinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ASinH_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{-5.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::ASinH(x);
    double r     = std::asinh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ASinH: max error = " << max_error << std::endl;
  // std::cout << "ASinH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACosH_16_16)
{
  fp32_t step{0.001};
  fp32_t x{1.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < 3.0; x += step)
  {
    fp32_t e     = fp32_t::ACosH(x);
    double r     = std::acosh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ACosH: max error = " << max_error << std::endl;
  // std::cout << "ACosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ACosH_32_32)
{
  fp64_t step{0.0001};
  fp64_t x{1.0};
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp64_t::TOLERANCE);
  for (; x < 5.0; x += step)
  {
    fp64_t e     = fp64_t::ACosH(x);
    double r     = std::acosh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ACosH: max error = " << max_error << std::endl;
  // std::cout << "ACosH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATanH_16_16)
{
  fp32_t step{0.001}, offset{step * 10};
  fp32_t x{-1.0}, max{1.0};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp32_t::TOLERANCE);
  for (; x < max; x += step)
  {
    fp32_t e     = fp32_t::ATanH(x);
    double r     = std::atanh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, 2 * tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ATamH: max error = " << max_error << std::endl;
  // std::cout << "ATanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, ATanH_32_32)
{
  fp64_t step{0.0001}, offset{step * 10};
  fp64_t x{-1.0}, max{1.0};
  x += offset;
  max -= offset;
  double max_error = 0, avg_error = 0;
  size_t iterations = 0;
  double tolerance  = 2 * static_cast<double>(fp64_t::TOLERANCE);
  for (; x < max; x += step)
  {
    fp64_t e     = fp64_t::ATanH(x);
    double r     = std::atanh(static_cast<double>(x));
    double delta = std::abs(static_cast<double>(e - r));
    max_error    = std::max(max_error, delta);
    avg_error += delta;
    iterations++;
  }
  avg_error /= static_cast<double>(iterations);
  EXPECT_NEAR(max_error, 0.0, tolerance);
  EXPECT_NEAR(avg_error, 0.0, tolerance);
  // std::cout << "ATanH: max error = " << max_error << std::endl;
  // std::cout << "ATanH: avg error = " << avg_error << std::endl;
}

TEST(FixedPointTest, NanInfinity_16_16)
{
  fp32_t m_inf{fp32_t::Constants.NEGATIVE_INFINITY};
  fp32_t p_inf{fp32_t::Constants.POSITIVE_INFINITY};

  // Basic checks
  EXPECT_TRUE(fp32_t::isInfinity(m_inf));
  EXPECT_TRUE(fp32_t::isNegInfinity(m_inf));
  EXPECT_TRUE(fp32_t::isInfinity(p_inf));
  EXPECT_TRUE(fp32_t::isPosInfinity(p_inf));
  EXPECT_FALSE(fp32_t::isNegInfinity(p_inf));
  EXPECT_FALSE(fp32_t::isPosInfinity(m_inf));

  // Absolute value
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::Abs(m_inf)));
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::Abs(p_inf)));
  EXPECT_EQ(fp32_t::Sign(m_inf), -fp32_t::_1);
  EXPECT_EQ(fp32_t::Sign(p_inf), fp32_t::_1);

  // Comparison checks
  EXPECT_FALSE(m_inf < m_inf);
  EXPECT_TRUE(m_inf <= m_inf);
  EXPECT_TRUE(m_inf < p_inf);
  EXPECT_TRUE(m_inf < fp32_t::_0);
  EXPECT_TRUE(m_inf < fp32_t::Constants.MIN);
  EXPECT_TRUE(m_inf < fp32_t::Constants.MAX);
  EXPECT_TRUE(m_inf < fp32_t::Constants.MAX);
  EXPECT_FALSE(p_inf > p_inf);
  EXPECT_TRUE(p_inf >= p_inf);
  EXPECT_TRUE(p_inf > m_inf);
  EXPECT_TRUE(p_inf > fp32_t::_0);
  EXPECT_TRUE(p_inf > fp32_t::Constants.MIN);
  EXPECT_TRUE(p_inf > fp32_t::Constants.MAX);

  // Addition checks
  // (-∞) + (-∞) = -∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(m_inf + m_inf));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (+∞) + (+∞) = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(p_inf + p_inf));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (-∞) + (+∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(m_inf + p_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // (+∞) + (-∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(p_inf + m_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // Subtraction checks
  // (-∞) - (+∞) = -∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(m_inf - p_inf));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (+∞) - (-∞) = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(p_inf - m_inf));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (-∞) - (-∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(m_inf - m_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // (+∞) - (+∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(p_inf - p_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // Multiplication checks
  // (-∞) * (+∞) = -∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(m_inf * p_inf));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (+∞) * (+∞) = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(p_inf * p_inf));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // 0 * (+∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::_0 * p_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // 0 * (-∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::_0 * m_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // Division checks
  // 0 / (+∞) = 0
  fp32_t::stateClear();
  EXPECT_EQ(fp32_t::_0 / p_inf, fp32_t::_0);
  // 0 * (-∞) = 0
  EXPECT_EQ(fp32_t::_0 / m_inf, fp32_t::_0);

  // (-∞) / MAX_INT = -∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(m_inf / fp32_t::Constants.MAX));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (+∞) / MAX_INT = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(p_inf / fp32_t::Constants.MAX));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (-∞) / MIN_INT = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(m_inf / fp32_t::Constants.MIN));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (+∞) / MIN_INT = -∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(p_inf / fp32_t::Constants.MIN));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (+∞) / (+∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(p_inf / p_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // (-∞) / (+∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(m_inf / p_inf));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // Exponential checks
  // e ^ (0/0) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Exp(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // e ^ (+∞) = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::Exp(p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // this is actually normal operation, does not modify the state
  // e ^ (-∞) = 0
  fp32_t::stateClear();
  EXPECT_EQ(fp32_t::Exp(m_inf), fp32_t::_0);

  // x^y checks
  // (-∞) ^ (-∞) = 0
  fp32_t::stateClear();
  EXPECT_EQ(fp32_t::Pow(m_inf, m_inf), fp32_t::_0);

  // (-∞) ^ 0 = 1
  fp32_t::stateClear();
  EXPECT_EQ(fp32_t::Pow(m_inf, fp32_t::_0), fp32_t::_1);

  // (+∞) ^ 0 = 1
  EXPECT_EQ(fp32_t::Pow(p_inf, fp32_t::_0), fp32_t::_1);

  // 0 ^ 0 = 1
  EXPECT_EQ(fp32_t::Pow(fp32_t::_0, fp32_t::_0), fp32_t::_1);

  // 0 ^ (-1) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Pow(fp32_t::_0, -fp32_t::_1)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // (-∞) ^ 1 = -∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(fp32_t::Pow(m_inf, fp32_t::_1)));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // (+∞) ^ 1 = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::Pow(p_inf, fp32_t::_1)));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // x ^ (+∞) = +∞, |x| > 1
  fp32_t x1{1.5};
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::Pow(x1, p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // x ^ (-∞) = 0, |x| > 1
  EXPECT_EQ(fp32_t::Pow(x1, m_inf), fp32_t::_0);

  // x ^ (+∞) = 0, |x| < 1
  fp32_t x2{0.5};
  EXPECT_EQ(fp32_t::Pow(x2, p_inf), fp32_t::_0);

  // x ^ (-∞) = 0, |x| < 1
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::Pow(x2, m_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // 1 ^ (-∞) = 1
  EXPECT_EQ(fp32_t::Pow(fp32_t::_1, m_inf), fp32_t::_1);

  // 1 ^ (-∞) = 1
  EXPECT_EQ(fp32_t::Pow(fp32_t::_1, p_inf), fp32_t::_1);

  // (-1) ^ (-∞) = 1
  EXPECT_EQ(fp32_t::Pow(-fp32_t::_1, m_inf), fp32_t::_1);

  // (-1) ^ (-∞) = 1
  EXPECT_EQ(fp32_t::Pow(-fp32_t::_1, p_inf), fp32_t::_1);

  // Logarithm checks
  // Log(NaN) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Log(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // Log(-∞) = NaN
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Log(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // Log(+∞) = +∞
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isInfinity(fp32_t::Log(p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // Trigonometry checks
  // Sin/Cos/Tan(NaN)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Sin(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Cos(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Tan(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // Sin/Cos/Tan(+/-∞)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Sin(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Sin(p_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Cos(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Cos(p_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Tan(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::Tan(p_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // ASin/ACos/ATan/ATan2(NaN)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ASin(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ACos(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ATan(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ATan2(fp32_t::_0 / fp32_t::_0, fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ATan2(fp32_t::_0, fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // ASin/ACos/ATan(+/-∞)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ASin(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ASin(p_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ACos(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ACos(p_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // ATan2(+/-∞)
  fp32_t::stateClear();
  EXPECT_EQ(fp32_t::ATan(m_inf), -fp32_t::Constants.PI_2);
  EXPECT_EQ(fp32_t::ATan(p_inf), fp32_t::Constants.PI_2);
  EXPECT_EQ(fp32_t::ATan2(fp32_t::_1, m_inf), fp32_t::Constants.PI);
  EXPECT_EQ(fp32_t::ATan2(-fp32_t::_1, m_inf), -fp32_t::Constants.PI);
  EXPECT_EQ(fp32_t::ATan2(fp32_t::_1, p_inf), fp32_t::_0);
  EXPECT_EQ(fp32_t::ATan2(m_inf, m_inf), -fp32_t::Constants.PI_4 * 3);
  EXPECT_EQ(fp32_t::ATan2(p_inf, m_inf), fp32_t::Constants.PI_4 * 3);
  EXPECT_EQ(fp32_t::ATan2(m_inf, p_inf), -fp32_t::Constants.PI_4);
  EXPECT_EQ(fp32_t::ATan2(p_inf, p_inf), fp32_t::Constants.PI_4);
  EXPECT_EQ(fp32_t::ATan2(m_inf, fp32_t::_1), -fp32_t::Constants.PI_2);
  EXPECT_EQ(fp32_t::ATan2(p_inf, fp32_t::_1), fp32_t::Constants.PI_2);

  // SinH/CosH/TanH(NaN)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::SinH(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::CosH(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::TanH(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // SinH/CosH/TanH(+/-∞)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(fp32_t::SinH(m_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::SinH(p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::CosH(m_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::CosH(p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(fp32_t::TanH(m_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::TanH(p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());

  // ASinH/ACosH/ATanH(NaN)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ASinH(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ACosH(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ATanH(fp32_t::_0 / fp32_t::_0)));
  EXPECT_TRUE(fp32_t::isStateNaN());

  // SinH/CosH/TanH(+/-∞)
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNegInfinity(fp32_t::ASinH(m_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::ASinH(p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ACosH(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isPosInfinity(fp32_t::ACosH(p_inf)));
  EXPECT_TRUE(fp32_t::isStateInfinity());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ATanH(m_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
  fp32_t::stateClear();
  EXPECT_TRUE(fp32_t::isNaN(fp32_t::ATanH(p_inf)));
  EXPECT_TRUE(fp32_t::isStateNaN());
}

TEST(FixedPointTest, NanInfinity_32_32)
{
  fp64_t m_inf{fp64_t::Constants.NEGATIVE_INFINITY};
  fp64_t p_inf{fp64_t::Constants.POSITIVE_INFINITY};

  // Basic checks
  EXPECT_TRUE(fp64_t::isInfinity(m_inf));
  EXPECT_TRUE(fp64_t::isNegInfinity(m_inf));
  EXPECT_TRUE(fp64_t::isInfinity(p_inf));
  EXPECT_TRUE(fp64_t::isPosInfinity(p_inf));
  EXPECT_FALSE(fp64_t::isNegInfinity(p_inf));
  EXPECT_FALSE(fp64_t::isPosInfinity(m_inf));

  // Absolute value
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::Abs(m_inf)));
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::Abs(p_inf)));
  EXPECT_EQ(fp64_t::Sign(m_inf), -fp64_t::_1);
  EXPECT_EQ(fp64_t::Sign(p_inf), fp64_t::_1);

  // Comparison checks
  EXPECT_FALSE(m_inf < m_inf);
  EXPECT_TRUE(m_inf <= m_inf);
  EXPECT_TRUE(m_inf < p_inf);
  EXPECT_TRUE(m_inf < fp64_t::_0);
  EXPECT_TRUE(m_inf < fp64_t::Constants.MIN);
  EXPECT_TRUE(m_inf < fp64_t::Constants.MAX);
  EXPECT_TRUE(m_inf < fp64_t::Constants.MAX);
  EXPECT_FALSE(p_inf > p_inf);
  EXPECT_TRUE(p_inf >= p_inf);
  EXPECT_TRUE(p_inf > m_inf);
  EXPECT_TRUE(p_inf > fp64_t::_0);
  EXPECT_TRUE(p_inf > fp64_t::Constants.MIN);
  EXPECT_TRUE(p_inf > fp64_t::Constants.MAX);

  // Addition checks
  // (-∞) + (-∞) = -∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(m_inf + m_inf));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (+∞) + (+∞) = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(p_inf + p_inf));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (-∞) + (+∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(m_inf + p_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // (+∞) + (-∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(p_inf + m_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // Subtraction checks
  // (-∞) - (+∞) = -∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(m_inf - p_inf));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (+∞) - (-∞) = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(p_inf - m_inf));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (-∞) - (-∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(m_inf - m_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // (+∞) - (+∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(p_inf - p_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // Multiplication checks
  // (-∞) * (+∞) = -∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(m_inf * p_inf));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (+∞) * (+∞) = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(p_inf * p_inf));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // 0 * (+∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::_0 * p_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // 0 * (-∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::_0 * m_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // Division checks
  // 0 / (+∞) = 0
  fp64_t::stateClear();
  EXPECT_EQ(fp64_t::_0 / p_inf, fp64_t::_0);
  // 0 * (-∞) = 0
  EXPECT_EQ(fp64_t::_0 / m_inf, fp64_t::_0);

  // (-∞) / MAX_INT = -∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(m_inf / fp64_t::Constants.MAX));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (+∞) / MAX_INT = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(p_inf / fp64_t::Constants.MAX));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (-∞) / MIN_INT = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(m_inf / fp64_t::Constants.MIN));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (+∞) / MIN_INT = -∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(p_inf / fp64_t::Constants.MIN));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (+∞) / (+∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(p_inf / p_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // (-∞) / (+∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(m_inf / p_inf));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // Exponential checks
  // e ^ (0/0) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Exp(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // e ^ (+∞) = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::Exp(p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // this is actually normal operation, does not modify the state
  // e ^ (-∞) = 0
  fp64_t::stateClear();
  EXPECT_EQ(fp64_t::Exp(m_inf), fp64_t::_0);

  // x^y checks
  // (-∞) ^ (-∞) = 0
  fp64_t::stateClear();
  EXPECT_EQ(fp64_t::Pow(m_inf, m_inf), fp64_t::_0);

  // (-∞) ^ 0 = 1
  fp64_t::stateClear();
  EXPECT_EQ(fp64_t::Pow(m_inf, fp64_t::_0), fp64_t::_1);

  // (+∞) ^ 0 = 1
  EXPECT_EQ(fp64_t::Pow(p_inf, fp64_t::_0), fp64_t::_1);

  // 0 ^ 0 = 1
  EXPECT_EQ(fp64_t::Pow(fp64_t::_0, fp64_t::_0), fp64_t::_1);

  // 0 ^ (-1) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Pow(fp64_t::_0, -fp64_t::_1)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // (-∞) ^ 1 = -∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(fp64_t::Pow(m_inf, fp64_t::_1)));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // (+∞) ^ 1 = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::Pow(p_inf, fp64_t::_1)));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // x ^ (+∞) = +∞, |x| > 1
  fp64_t x1{1.5};
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::Pow(x1, p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // x ^ (-∞) = 0, |x| > 1
  EXPECT_EQ(fp64_t::Pow(x1, m_inf), fp64_t::_0);

  // x ^ (+∞) = 0, |x| < 1
  fp64_t x2{0.5};
  EXPECT_EQ(fp64_t::Pow(x2, p_inf), fp64_t::_0);

  // x ^ (-∞) = 0, |x| < 1
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::Pow(x2, m_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // 1 ^ (-∞) = 1
  EXPECT_EQ(fp64_t::Pow(fp64_t::_1, m_inf), fp64_t::_1);

  // 1 ^ (-∞) = 1
  EXPECT_EQ(fp64_t::Pow(fp64_t::_1, p_inf), fp64_t::_1);

  // (-1) ^ (-∞) = 1
  EXPECT_EQ(fp64_t::Pow(-fp64_t::_1, m_inf), fp64_t::_1);

  // (-1) ^ (-∞) = 1
  EXPECT_EQ(fp64_t::Pow(-fp64_t::_1, p_inf), fp64_t::_1);

  // Logarithm checks
  // Log(NaN) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Log(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // Log(-∞) = NaN
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Log(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // Log(+∞) = +∞
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isInfinity(fp64_t::Log(p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // Trigonometry checks
  // Sin/Cos/Tan(NaN)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Sin(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Cos(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Tan(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // Sin/Cos/Tan(+/-∞)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Sin(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Sin(p_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Cos(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Cos(p_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Tan(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::Tan(p_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // ASin/ACos/ATan/ATan2(NaN)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ASin(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ACos(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ATan(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ATan2(fp64_t::_0 / fp64_t::_0, fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ATan2(fp64_t::_0, fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // ASin/ACos/ATan(+/-∞)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ASin(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ASin(p_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ACos(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ACos(p_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // ATan2(+/-∞)
  fp64_t::stateClear();
  EXPECT_EQ(fp64_t::ATan(m_inf), -fp64_t::Constants.PI_2);
  EXPECT_EQ(fp64_t::ATan(p_inf), fp64_t::Constants.PI_2);
  EXPECT_EQ(fp64_t::ATan2(fp64_t::_1, m_inf), fp64_t::Constants.PI);
  EXPECT_EQ(fp64_t::ATan2(-fp64_t::_1, m_inf), -fp64_t::Constants.PI);
  EXPECT_EQ(fp64_t::ATan2(fp64_t::_1, p_inf), fp64_t::_0);
  EXPECT_EQ(fp64_t::ATan2(m_inf, m_inf), -fp64_t::Constants.PI_4 * 3);
  EXPECT_EQ(fp64_t::ATan2(p_inf, m_inf), fp64_t::Constants.PI_4 * 3);
  EXPECT_EQ(fp64_t::ATan2(m_inf, p_inf), -fp64_t::Constants.PI_4);
  EXPECT_EQ(fp64_t::ATan2(p_inf, p_inf), fp64_t::Constants.PI_4);
  EXPECT_EQ(fp64_t::ATan2(m_inf, fp64_t::_1), -fp64_t::Constants.PI_2);
  EXPECT_EQ(fp64_t::ATan2(p_inf, fp64_t::_1), fp64_t::Constants.PI_2);

  // SinH/CosH/TanH(NaN)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::SinH(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::CosH(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::TanH(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // SinH/CosH/TanH(+/-∞)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(fp64_t::SinH(m_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::SinH(p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::CosH(m_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::CosH(p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(fp64_t::TanH(m_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::TanH(p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());

  // ASinH/ACosH/ATanH(NaN)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ASinH(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ACosH(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ATanH(fp64_t::_0 / fp64_t::_0)));
  EXPECT_TRUE(fp64_t::isStateNaN());

  // SinH/CosH/TanH(+/-∞)
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNegInfinity(fp64_t::ASinH(m_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::ASinH(p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ACosH(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isPosInfinity(fp64_t::ACosH(p_inf)));
  EXPECT_TRUE(fp64_t::isStateInfinity());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ATanH(m_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
  fp64_t::stateClear();
  EXPECT_TRUE(fp64_t::isNaN(fp64_t::ATanH(p_inf)));
  EXPECT_TRUE(fp64_t::isStateNaN());
}
