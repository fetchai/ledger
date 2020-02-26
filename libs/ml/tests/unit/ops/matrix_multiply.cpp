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

#include "test_types.hpp"

#include "ml/ops/matrix_multiply.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using namespace fetch::ml;

template <typename T>
class MatrixMultiplyTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MatrixMultiplyTest, fetch::math::test::TensorIntAndFloatingTypes, );

TYPED_TEST(MatrixMultiplyTest, forward_test)
{
  TypeParam a = TypeParam::FromString(R"(1, 2, -3, 4, 5)");
  TypeParam b = TypeParam::FromString(
      R"(-11, 12, 13, 14; 21, 22, 23, 24; 31, 32, 33, 34; 41, 42, 43, 44; 51, 52, 53, 54)");
  TypeParam gt = TypeParam::FromString(R"(357, 388, 397, 406)");

  fetch::ml::ops::MatrixMultiply<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({a.shape(), b.shape()}));
  op.Forward({std::make_shared<TypeParam>(a), std::make_shared<TypeParam>(b)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), std::vector<typename TypeParam::SizeType>({1, 4}));
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(MatrixMultiplyTest, backward_test)
{
  TypeParam a = TypeParam::FromString(R"(1, 2, -3, 4, 5)");
  TypeParam b = TypeParam::FromString(
      R"(-11, 12, 13, 14; 21, 22, 23, 24; 31, 32, 33, 34; 41, 42, 43, 44; 51, 52, 53, 54)");
  TypeParam error      = TypeParam::FromString(R"(1, 2, 3, -4)");
  TypeParam gradient_a = TypeParam::FromString(R"(-4, 38, 58, 78, 98)");
  TypeParam gradient_b = TypeParam::FromString(
      R"(1, 2, 3, -4; 2, 4, 6, -8; -3, -6, -9, 12; 4, 8, 12, -16; 5, 10, 15, -20)");

  fetch::ml::ops::MatrixMultiply<TypeParam> op;
  std::vector<TypeParam>                    backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a), std::make_shared<TypeParam>(b)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 2);
  ASSERT_EQ(backpropagated_signals[0].shape(), std::vector<typename TypeParam::SizeType>({1, 5}));
  ASSERT_EQ(backpropagated_signals[1].shape(), std::vector<typename TypeParam::SizeType>({5, 4}));

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gradient_a));
  EXPECT_TRUE(backpropagated_signals[1].AllClose(gradient_b));
}

TYPED_TEST(MatrixMultiplyTest, forward_batch_test)
{

  TypeParam a({3, 4, 2});
  TypeParam b({4, 3, 2});
  TypeParam gt({3, 3, 2});

  fetch::ml::ops::MatrixMultiply<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({a.shape(), b.shape()}));
  op.Forward({std::make_shared<TypeParam>(a), std::make_shared<TypeParam>(b)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), std::vector<typename TypeParam::SizeType>({3, 3, 2}));
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(MatrixMultiplyTest, backward_batch_test)
{
  TypeParam a({3, 4, 2});
  TypeParam b({4, 3, 2});
  TypeParam error({3, 3, 2});
  TypeParam gradient_a({3, 4, 2});
  TypeParam gradient_b({4, 3, 2});

  fetch::ml::ops::MatrixMultiply<TypeParam> op;
  std::vector<TypeParam>                    backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a), std::make_shared<TypeParam>(b)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 2);
  ASSERT_EQ(backpropagated_signals[0].shape(),
            std::vector<typename TypeParam::SizeType>({3, 4, 2}));
  ASSERT_EQ(backpropagated_signals[1].shape(),
            std::vector<typename TypeParam::SizeType>({4, 3, 2}));

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gradient_a));
  EXPECT_TRUE(backpropagated_signals[1].AllClose(gradient_b));
}

}  // namespace
