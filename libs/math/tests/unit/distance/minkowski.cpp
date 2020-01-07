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

#include "core/random/lcg.hpp"
#include "math/distance/euclidean.hpp"
#include "math/distance/manhattan.hpp"
#include "math/distance/minkowski.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class MinkowskiTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MinkowskiTest, TensorFloatingTypes);

TYPED_TEST(MinkowskiTest, simple_test)
{
  using Type = typename TypeParam::Type;

  TypeParam a   = TypeParam::FromString("1, 0, 0");
  TypeParam b   = TypeParam::FromString("0, 1, 0");
  Type      ret = distance::Minkowski(a, b, Type{2});
  EXPECT_NEAR(static_cast<double>(ret), 1.41421356237,
              static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 0, 0");
  b   = TypeParam::FromString("0, 1, 0");
  ret = distance::Minkowski(a, b, Type{3});
  EXPECT_NEAR(static_cast<double>(ret), 1.25992104989,
              static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 5, 7");
  ret = distance::Minkowski(a, a, Type{3});
  EXPECT_NEAR(static_cast<double>(ret), 0, static_cast<double>(function_tolerance<Type>()));

  // minkowski with lambda=1 == manhattan
  a         = TypeParam::FromString("1, 2, 3");
  b         = TypeParam::FromString("10, 11, 12");
  ret       = distance::Minkowski(a, b, Type{1});
  Type ret2 = distance::Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), static_cast<double>(ret2),
              static_cast<double>(function_tolerance<Type>()));

  // minkowski with lambda=2 == euclidean
  a    = TypeParam::FromString("1, 0, 0");
  b    = TypeParam::FromString("0, 1, 0");
  ret  = distance::Minkowski(a, b, Type{2});
  ret2 = distance::Euclidean(a, b);
  EXPECT_NEAR(static_cast<double>(ret), static_cast<double>(ret2),
              static_cast<double>(function_tolerance<Type>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
