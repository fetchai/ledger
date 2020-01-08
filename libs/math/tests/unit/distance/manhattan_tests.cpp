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
#include "gtest/gtest.h"
#include "math/distance/manhattan.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class ManhattanTest : public ::testing::Test
{
};
TYPED_TEST_CASE(ManhattanTest, TensorFloatingTypes);

TYPED_TEST(ManhattanTest, simple_test)
{
  using Type = typename TypeParam::Type;

  TypeParam a   = TypeParam::FromString("1, 0, 0");
  TypeParam b   = TypeParam::FromString("0, 1, 0");
  Type      ret = distance::Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), 2, static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("2, 3, 5");
  ret = distance::Manhattan(a, a);
  EXPECT_NEAR(static_cast<double>(ret), 0, static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 0, 0");
  b   = TypeParam::FromString("0, 2, 0");
  ret = distance::Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), 3, static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 0, 0");
  b   = TypeParam::FromString("1, 1, 0");
  ret = distance::Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), 1, static_cast<double>(function_tolerance<Type>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
