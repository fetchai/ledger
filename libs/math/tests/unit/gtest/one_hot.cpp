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

#include "math/base_types.hpp"
#include "math/one_hot.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class OneHotTest : public ::testing::Test
{
};

TYPED_TEST_CASE(OneHotTest, TensorFloatingTypes);

TYPED_TEST(OneHotTest, one_hot_test_axis_0)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({4});
  ArrayType gt = TypeParam::FromString("-1, 5, -1, -1; 5, -1, 5, -1; -1, -1, -1, 5");
  gt.Reshape({3, 4});

  SizeType depth     = 3;
  auto     on_value  = fetch::math::Type<DataType>("5.0");
  auto     off_value = fetch::math::Type<DataType>("-1.0");

  ArrayType ret = OneHot(data, depth, 0, on_value, off_value);

  ASSERT_EQ(ret.shape(), gt.shape());
  ASSERT_TRUE(ret.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                           fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(OneHotTest, one_hot_test_axis_1)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({4});
  ArrayType gt = TypeParam::FromString("-1, 5, -1; 5, -1, -1; -1, 5, -1; -1, -1, 5");
  gt.Reshape({4, 3});

  SizeType depth     = 3;
  auto     on_value  = fetch::math::Type<DataType>("5.0");
  auto     off_value = fetch::math::Type<DataType>("-1.0");

  ArrayType ret = OneHot(data, depth, 1, on_value, off_value);

  ASSERT_EQ(ret.shape(), gt.shape());
  ASSERT_TRUE(ret.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                           fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(OneHotTest, one_hot_test_axis_3)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({2, 2, 1, 1});
  ArrayType gt = TypeParam::FromString("-1, 5, -1; 5, -1, -1; -1, 5, -1; -1, -1, 5");
  gt.Reshape({2, 2, 1, 3, 1});

  SizeType depth     = 3;
  auto     on_value  = fetch::math::Type<DataType>("5.0");
  auto     off_value = fetch::math::Type<DataType>("-1.0");

  ArrayType ret = OneHot(data, depth, 3, on_value, off_value);

  ASSERT_EQ(ret.shape(), gt.shape());
  ASSERT_TRUE(ret.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                           fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
