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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "ml/core/graph.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/reduce_mean.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class ReduceMeanTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ReduceMeanTest, math::test::TensorFloatingTypes);

TYPED_TEST(ReduceMeanTest, forward_2_2_2_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  TensorType gt = TensorType::FromString("2.5, 5, 0, 400");
  gt.Reshape({2, 1, 2});

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape().at(0), 2);
  ASSERT_EQ(prediction.shape().at(1), 1);
  ASSERT_EQ(prediction.shape().at(2), 2);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ReduceMeanTest, backward_2_2_2_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({2, 1, 2});
  TensorType gt_error = TensorType::FromString("0.5, -1, 0.5, -1, -0.5, 1, -0.5, 1");
  gt_error.Reshape({2, 2, 2});

  auto shape = error.shape();

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_EQ(error_signal.at(0).shape().size(), 3);
  ASSERT_EQ(error_signal.at(0).shape().at(0), 2);
  ASSERT_EQ(error_signal.at(0).shape().at(1), 2);
  ASSERT_EQ(error_signal.at(0).shape().at(2), 2);

  ASSERT_TRUE(error_signal.at(0).AllClose(gt_error, fetch::math::function_tolerance<DataType>(),
                                          fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
