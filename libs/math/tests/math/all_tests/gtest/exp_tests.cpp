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

#include <chrono>
#include <cmath>
#include <iostream>

#include "core/random/lcg.hpp"

#include "math/approx_exp.hpp"
#include "math/fixed_point/fixed_point.hpp"
#include <gtest/gtest.h>

// TODO: NOTES: should this work with float?
// TODO: NOTES: should it work with FixedPoint?
// TODO: which tests may not be necessary?

template <uint8_t N, uint64_t C, typename T>
void test1(T max)
{
  fetch::math::ApproxExpImplementation<N, C> fexp;
  double                                     me = 0;
  for (double x = -300; x < 300; x += 0.1)
  {
    T y0 = static_cast<T>(fexp(static_cast<T>(x)));
    //    T y0 = fexp(x);
    double y1 = exp(x);
    double r  = (fabs(static_cast<double>(y0) - y1) / y1 * 100);
    //    double r  = (fabs(y0 - y1) / y1 * 100);
    me = std::max(r, me);
  }
  //  ASSERT_LE(me, static_cast<double>(max)) << "expected: " << static_cast<double>(max);
  EXPECT_LE(me, static_cast<double>(max));
}

template <typename T>
class ExpTests : public ::testing::Test
{
};

// using MyTypes = ::testing::Types<float, double, fetch::fixed_point::FixedPoint<32, 32>>;
using MyTypes = ::testing::Types<float, double>;
TYPED_TEST_CASE(ExpTests, MyTypes);

TYPED_TEST(ExpTests, exp_0_0)
{
  TypeParam x = TypeParam(7);
  test1<0, 0>(x);
}

TYPED_TEST(ExpTests, exp_0_60801)
{
  TypeParam x = TypeParam{5};
  test1<0, 60801>(x);
}

TYPED_TEST(ExpTests, exp_8_60801)
{
  TypeParam x = TypeParam(0.08);
  test1<8, 60801>(x);
}

TYPED_TEST(ExpTests, exp_12_60801)
{
  TypeParam x = TypeParam(0.005);
  test1<12, 60801>(x);
}

TYPED_TEST(ExpTests, exp_16_60801)
{
  TypeParam x = TypeParam(0.0003);
  test1<16, 60801>(x);
}

TYPED_TEST(ExpTests, exp_20_60801)
{
  TypeParam x = TypeParam(0.00004);
  test1<20, 60801>(x);
}

TYPED_TEST(ExpTests, exp_8_0)
{
  TypeParam x = TypeParam(0.08);
  test1<8, 0>(x);
}

TYPED_TEST(ExpTests, exp_12_0)
{
  TypeParam x = TypeParam(0.005);
  test1<12, 0>(x);
}

TYPED_TEST(ExpTests, exp_16_0)
{
  TypeParam x = TypeParam(0.0003);
  test1<16, 0>(x);
}

TYPED_TEST(ExpTests, exp_20_0)
{
  TypeParam x = TypeParam(0.00004);
  test1<20, 0>(x);
}