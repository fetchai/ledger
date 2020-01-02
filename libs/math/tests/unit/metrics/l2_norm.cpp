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
#include "math/metrics/l2_norm.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class L2NormTest : public ::testing::Test
{
};

TYPED_TEST_CASE(L2NormTest, TensorFloatingTypes);

TYPED_TEST(L2NormTest, value_test)
{
  using DataType       = typename TypeParam::Type;
  TypeParam test_array = TypeParam{8};

  test_array[0] = DataType(1);
  test_array[1] = DataType(-2);
  test_array[2] = DataType(3);
  test_array[3] = DataType(-4);
  test_array[4] = DataType(5);
  test_array[5] = DataType(-6);
  test_array[6] = DataType(7);
  test_array[7] = DataType(-8);

  DataType score = fetch::math::L2Norm(test_array);

  // test correct values
  EXPECT_NEAR(double(score), double(14.282856857085700852),
              static_cast<double>(function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
