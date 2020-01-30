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
#include "ml/ops/top_k.hpp"

#include "gtest/gtest.h"
#include "test_types.hpp"

#include <memory>
#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class TopKOpTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TopKOpTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(TopKOpTest, forward_test)
{
  using SizeType   = typename TypeParam::SizeType;
  using TensorType = TypeParam;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  TensorType gt = TypeParam::FromString("13,14,15,16;9,10,11,12");
  gt.Reshape({2, 4});

  SizeType k      = 2;
  bool     sorted = true;

  fetch::ml::ops::TopK<TypeParam> op(k, sorted);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(data)}));
  op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(TopKOpTest, backward_2D_test)
{
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using DataType   = typename TypeParam::Type;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  TensorType error = TypeParam::FromString("20,-21,22,-23;24,-25,26,-27");
  error.Reshape({2, 4});
  TensorType gt_error = TypeParam::FromString("24,0,0,0;0,0,0,0;0,-25,26,-27;20,-21,22,-23");
  gt_error.Reshape({4, 4});

  SizeType k      = 2;
  bool     sorted = true;

  auto shape = error.shape();

  fetch::ml::ops::TopK<TypeParam> op(k, sorted);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(data)}));
  op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_EQ(error_signal.at(0).shape().size(), 2);
  ASSERT_EQ(error_signal.at(0).shape().at(0), 4);
  ASSERT_EQ(error_signal.at(0).shape().at(1), 4);

  ASSERT_TRUE(error_signal.at(0).AllClose(gt_error, fetch::math::function_tolerance<DataType>(),
                                          fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}

}  // namespace
