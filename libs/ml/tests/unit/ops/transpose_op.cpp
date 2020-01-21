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

#include "ml/ops/transpose.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class TransposeTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TransposeTest, fetch::math::test::TensorIntAndFloatingTypes);

TYPED_TEST(TransposeTest, forward_test)
{
  TypeParam a  = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");
  TypeParam gt = TypeParam::FromString(R"(1, 4; 2, 5; -3, 6)");

  fetch::ml::ops::Transpose<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(TransposeTest, backward_test)
{
  TypeParam a        = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");
  TypeParam error    = TypeParam::FromString(R"(1, 4; 2, 5; -3, 6)");
  TypeParam gradient = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");

  fetch::ml::ops::Transpose<TypeParam> op;
  std::vector<TypeParam>               backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), a.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gradient));
}

TYPED_TEST(TransposeTest, forward_batch_test)
{
  TypeParam a({4, 5, 2});
  TypeParam gt({5, 4, 2});

  fetch::ml::ops::Transpose<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(TransposeTest, backward_batch_test)
{
  TypeParam a({4, 5, 2});
  TypeParam error({5, 4, 2});
  TypeParam gradient({4, 5, 2});

  fetch::ml::ops::Transpose<TypeParam> op;
  std::vector<TypeParam>               backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), a.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gradient));
}

}  // namespace
