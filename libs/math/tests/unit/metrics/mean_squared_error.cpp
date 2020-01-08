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

#include "gtest/gtest.h"
#include "math/metrics/mean_square_error.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class MeanSquareErrorTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MeanSquareErrorTest, TensorFloatingTypes);

TYPED_TEST(MeanSquareErrorTest, perfect_match_test)
{
  using DataType       = typename TypeParam::Type;
  TypeParam test_array = TypeParam{8};
  TypeParam gt_array   = TypeParam{8};

  test_array[0] = fetch::math::Type<DataType>("1");
  test_array[1] = fetch::math::Type<DataType>("-2");
  test_array[2] = fetch::math::Type<DataType>("3");
  test_array[3] = fetch::math::Type<DataType>("-4");
  test_array[4] = fetch::math::Type<DataType>("5");
  test_array[5] = fetch::math::Type<DataType>("-6");
  test_array[6] = fetch::math::Type<DataType>("7");
  test_array[7] = fetch::math::Type<DataType>("-8");

  gt_array[0] = fetch::math::Type<DataType>("1");
  gt_array[1] = fetch::math::Type<DataType>("-2");
  gt_array[2] = fetch::math::Type<DataType>("3");
  gt_array[3] = fetch::math::Type<DataType>("-4");
  gt_array[4] = fetch::math::Type<DataType>("5");
  gt_array[5] = fetch::math::Type<DataType>("-6");
  gt_array[6] = fetch::math::Type<DataType>("7");
  gt_array[7] = fetch::math::Type<DataType>("-8");

  // initialise to non-zero just to avoid correct value at initialisation
  DataType score(100);
  score = fetch::math::MeanSquareError(test_array, gt_array);

  // test correct values
  ASSERT_NEAR(double(score), double(0.0), static_cast<double>(function_tolerance<DataType>()));
}

TYPED_TEST(MeanSquareErrorTest, value_test)
{
  using DataType = typename TypeParam::Type;

  TypeParam test_array = TypeParam{8};
  TypeParam gt_array   = TypeParam{8};

  test_array[0] = fetch::math::Type<DataType>("1.1");
  test_array[1] = fetch::math::Type<DataType>("-2.2");
  test_array[2] = fetch::math::Type<DataType>("3.3");
  test_array[3] = fetch::math::Type<DataType>("-4.4");
  test_array[4] = fetch::math::Type<DataType>("5.5");
  test_array[5] = fetch::math::Type<DataType>("-6.6");
  test_array[6] = fetch::math::Type<DataType>("7.7");
  test_array[7] = fetch::math::Type<DataType>("-8.8");

  gt_array[0] = fetch::math::Type<DataType>("1.1");
  gt_array[1] = fetch::math::Type<DataType>("2.2");
  gt_array[2] = fetch::math::Type<DataType>("7.7");
  gt_array[3] = fetch::math::Type<DataType>("6.6");
  gt_array[4] = fetch::math::Type<DataType>("0.0");
  gt_array[5] = fetch::math::Type<DataType>("-6.6");
  gt_array[6] = fetch::math::Type<DataType>("7.7");
  gt_array[7] = fetch::math::Type<DataType>("-9.9");

  // initialise to non-zero just to avoid correct value at initialisation
  DataType score(0);
  score = fetch::math::MeanSquareError(test_array, gt_array);

  // test correct values
  ASSERT_NEAR(double(score), double(191.18f / 8.0f),
              8.0 * static_cast<double>(function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
