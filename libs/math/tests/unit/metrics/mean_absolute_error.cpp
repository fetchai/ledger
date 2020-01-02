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
#include "math/metrics/mean_absolute_error.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class MeanAbsoluteErrorTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MeanAbsoluteErrorTest, TensorFloatingTypes);

TYPED_TEST(MeanAbsoluteErrorTest, perfect_match_test)
{
  TypeParam test_array = TypeParam::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TypeParam gt_array   = TypeParam::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  // initialise to non-zero just to avoid correct value at initialisation
  typename TypeParam::Type score(100);
  score = fetch::math::MeanAbsoluteError(test_array, gt_array);

  // test correct values
  ASSERT_NEAR(static_cast<double>(score), double(0.0),
              static_cast<double>(test_array.size()) *
                  static_cast<double>(fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(MeanAbsoluteErrorTest, value_test)
{
  TypeParam test_array = TypeParam::FromString("1.1, -2.2, 3.3, -4.4, 5.5, -6.6, 7.7, -8.8");
  TypeParam gt_array   = TypeParam::FromString("1.1, 2.2, 7.7, 6.6, 0.0, -6.6, 7.7, -9.9");

  // initialise to zero just to avoid correct value at initialisation
  typename TypeParam::Type score(0);
  score = fetch::math::MeanAbsoluteError(test_array, gt_array);

  // test correct values
  ASSERT_NEAR(static_cast<double>(score), double(3.3f),
              static_cast<double>(test_array.size()) *
                  static_cast<double>(fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
