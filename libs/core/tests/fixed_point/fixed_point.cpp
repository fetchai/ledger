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
}

TEST(FixedPointTest, Multiplication)
{
  // Positive
  fetch::fixed_point::FixedPoint<32, 32> zero(0);
  fetch::fixed_point::FixedPoint<32, 32> one(1);
  fetch::fixed_point::FixedPoint<32, 32> two(2);

  EXPECT_EQ(int(two * one), 2);
  EXPECT_EQ(float(two * one), 2.0f);
  EXPECT_EQ(double(two * one), 2.0);

  EXPECT_EQ(int(one * two), 2);
  EXPECT_EQ(float(one * two), 2.0f);
  EXPECT_EQ(double(one * two), 2.0);

  EXPECT_EQ(int(two * zero), 0);
  EXPECT_EQ(float(two * zero), 0.0f);
  EXPECT_EQ(double(two * zero), 0.0);
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

  // ??? -- What should the behaviour be here ?
  /*
    EXPECT_EQ(int(two / zero), 0);
    EXPECT_EQ(float(two / zero), 0.0f);
    EXPECT_EQ(double(two / zero), 0.0);
  */
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
