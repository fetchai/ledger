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

#include "core/random/lcg.hpp"
#include "math/distance/euclidean.hpp"
#include "math/distance/manhattan.hpp"
#include "math/distance/minkowski.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

namespace {

using namespace fetch::math::distance;
using namespace fetch::math;

template <typename T>
class MinkowskiTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(MinkowskiTest, MyTypes);

TYPED_TEST(MinkowskiTest, simple_test)
{
  using Type = typename TypeParam::Type;

  TypeParam a   = TypeParam::FromString("1, 0, 0");
  TypeParam b   = TypeParam::FromString("0, 1, 0");
  Type      ret = Minkowski(a, b, Type{2});
  EXPECT_NEAR(static_cast<double>(ret), 1.41421356237,
              static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 0, 0");
  b   = TypeParam::FromString("0, 1, 0");
  ret = Minkowski(a, b, Type{3});
  EXPECT_NEAR(static_cast<double>(ret), 1.25992104989,
              static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 5, 7");
  ret = Minkowski(a, a, Type{3});
  EXPECT_NEAR(static_cast<double>(ret), 0, static_cast<double>(function_tolerance<Type>()));

  // minkowski with lambda=1 == manhattan
  a         = TypeParam::FromString("1, 2, 3");
  b         = TypeParam::FromString("10, 11, 12");
  ret       = Minkowski(a, b, Type{1});
  Type ret2 = Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), static_cast<double>(ret2),
              static_cast<double>(function_tolerance<Type>()));

  // minkowski with lambda=2 == euclidean
  a    = TypeParam::FromString("1, 0, 0");
  b    = TypeParam::FromString("0, 1, 0");
  ret  = Minkowski(a, b, Type{2});
  ret2 = Euclidean(a, b);
  EXPECT_NEAR(static_cast<double>(ret), static_cast<double>(ret2),
              static_cast<double>(function_tolerance<Type>()));
}
}  // namespace
