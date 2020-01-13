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
#include "ml/ops/log.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <vector>

namespace {
template <typename T>
class LogTest : public ::testing::Test
{
};

TYPED_TEST_CASE(LogTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(LogTest, forward_all_positive_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  TensorType gt   = TensorType::FromString(
      "0, 0.693147180559945, 1.38629436111989, 2.07944154167984, 4.60517018598809, "
      "6.90775527898214");

  fetch::ml::ops::Log<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(LogTest, forward_all_negative_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("-1, -2, -4, -10, -100");

  fetch::ml::ops::Log<TypeParam> op;

  TypeParam pred(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, pred);

  // gives NaN because log of a negative number is undefined
  for (auto const &p_it : pred)
  {
    EXPECT_TRUE(fetch::math::is_nan<DataType>(p_it));
  }
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(LogTest, backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data  = TensorType::FromString("1, -2, 4, -10, 100");
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  // (1 / data) * error = gt
  TensorType gt = TensorType::FromString("1,-0.5,0.25,-0.2,0");

  fetch::ml::ops::Log<TypeParam> op;

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_TRUE(prediction.at(0).AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}

}  // namespace
