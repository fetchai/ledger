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
#include "math/distance/manhattan.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

namespace {
using namespace fetch::math::distance;
using namespace fetch::math;

template <typename T>
class ManhattanTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(ManhattanTest, MyTypes);

TYPED_TEST(ManhattanTest, simple_test)
{
  using Type = typename TypeParam::Type;

  TypeParam a   = TypeParam::FromString("1, 0, 0");
  TypeParam b   = TypeParam::FromString("0, 1, 0");
  Type      ret = Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), 2, static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("2, 3, 5");
  ret = Manhattan(a, a);
  EXPECT_NEAR(static_cast<double>(ret), 0, static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 0, 0");
  b   = TypeParam::FromString("0, 2, 0");
  ret = Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), 3, static_cast<double>(function_tolerance<Type>()));

  a   = TypeParam::FromString("1, 0, 0");
  b   = TypeParam::FromString("1, 1, 0");
  ret = Manhattan(a, b);
  EXPECT_NEAR(static_cast<double>(ret), 1, static_cast<double>(function_tolerance<Type>()));
}
}  // namespace
