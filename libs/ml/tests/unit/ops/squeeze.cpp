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
#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "ml/core/graph.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/squeeze.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SqueezeTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SqueezeTest, math::test::TensorFloatingTypes);

TYPED_TEST(SqueezeTest, forward_1_6_1_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({1, 6, 1});

  fetch::ml::ops::Squeeze<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape().at(0), 1);
  ASSERT_EQ(prediction.shape().at(1), 6);

  ASSERT_TRUE(prediction.AllClose(data, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  TypeParam prediction2(op.ComputeOutputShape({std::make_shared<const TensorType>(prediction)}));
  op.Forward({std::make_shared<const TensorType>(prediction)}, prediction2);

  ASSERT_EQ(prediction2.shape().size(), 1);
  ASSERT_EQ(prediction2.shape().at(0), 6);

  ASSERT_TRUE(prediction2.AllClose(data, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SqueezeTest, forward_throw_test)
{
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6});

  fetch::ml::ops::Squeeze<TypeParam> op;

  EXPECT_ANY_THROW(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  TypeParam prediction({6});
  EXPECT_ANY_THROW(op.Forward({std::make_shared<const TensorType>(data)}, prediction));
}

TYPED_TEST(SqueezeTest, backward_1_5_1_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5, 1});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({1, 5});

  auto shape = error.shape();

  fetch::ml::ops::Squeeze<TypeParam> op;

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_EQ(error_signal.at(0).shape().size(), 3);
  ASSERT_EQ(error_signal.at(0).shape().at(0), 1);
  ASSERT_EQ(error_signal.at(0).shape().at(1), 5);
  ASSERT_EQ(error_signal.at(0).shape().at(2), 1);

  ASSERT_TRUE(error_signal.at(0).AllClose(error, fetch::math::function_tolerance<DataType>(),
                                          fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqueezeTest, backward_1_5_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({5});

  auto shape = error.shape();

  fetch::ml::ops::Squeeze<TypeParam> op;

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_EQ(error_signal.at(0).shape().size(), 2);
  ASSERT_EQ(error_signal.at(0).shape().at(0), 1);
  ASSERT_EQ(error_signal.at(0).shape().at(1), 5);

  ASSERT_TRUE(error_signal.at(0).AllClose(error, fetch::math::function_tolerance<DataType>(),
                                          fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}


}  // namespace test
}  // namespace ml
}  // namespace fetch
