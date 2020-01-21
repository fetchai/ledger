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
#include "ml/ops/sqrt.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class SqrtTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SqrtTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(SqrtTest, forward_all_positive_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data = TensorType::FromString("0, 1, 2, 4, 10, 100");
  TensorType gt   = TensorType::FromString("0, 1, 1.41421356, 2, 3.1622776, 10");

  fetch::ml::ops::Sqrt<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SqrtTest, backward_all_positive_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data  = TensorType::FromString("1,   2,         4,   10,       100");
  TensorType error = TensorType::FromString("1,   1,         1,    2,         0");
  // 0.5 / sqrt(data) * error = gt
  TensorType gt = TensorType::FromString("0.5, 0.3535533, 0.25, 0.3162277, 0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_TRUE(prediction.at(0).AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SqrtTest, forward_all_negative_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data = TensorType::FromString("-1, -2, -4, -10, -100");

  fetch::ml::ops::Sqrt<TypeParam> op;

  TypeParam pred(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, pred);

  // gives NaN because sqrt of a negative number is undefined
  for (auto const &p_it : pred)
  {
    EXPECT_TRUE(fetch::math::is_nan(p_it));
  }
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqrtTest, backward_all_negative_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data  = TensorType::FromString("-1, -2, -4, -10, -100");
  TensorType error = TensorType::FromString("1,   1,  1,   2,    0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<TensorType> pred = op.Backward({std::make_shared<const TensorType>(data)}, error);
  // gives NaN because sqrt of a negative number is undefined
  for (auto const &p_it : pred.at(0))
  {
    EXPECT_TRUE(fetch::math::is_nan(p_it));
  }
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqrtTest, backward_zero_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data  = TensorType::FromString("0,  0,    0,    0,        0");
  TensorType error = TensorType::FromString("1,  1,    1,    2,        0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<TensorType> pred = op.Backward({std::make_shared<const TensorType>(data)}, error);
  // gives NaN because of division by zero
  for (auto const &p_it : pred.at(0))
  {
    EXPECT_TRUE(fetch::math::is_inf(p_it) || fetch::math::is_nan(p_it));
  }
  fetch::math::state_clear<DataType>();
}

}  // namespace
