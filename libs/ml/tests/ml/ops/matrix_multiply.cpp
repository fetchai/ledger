//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include "math/tensor.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

template <typename T>
class MatrixMultiplyTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(MatrixMultiplyTest, MyTypes);

TYPED_TEST(MatrixMultiplyTest, forward_test)
{
  TypeParam a = TypeParam::FromString(R"(1, 2, -3, 4, 5)");
  TypeParam b = TypeParam::FromString(
      R"(-11, 12, 13, 14; 21, 22, 23, 24; 31, 32, 33, 34; 41, 42, 43, 44; 51, 52, 53, 54)");
  TypeParam gt = TypeParam::FromString(R"(357, 388, 397, 406)");

  fetch::ml::ops::MatrixMultiply<TypeParam> op;

  TypeParam prediction(
      op.ComputeOutputShape({std::make_shared<TypeParam>(a), std::make_shared<TypeParam>(b)}));
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

  TypeParam prediction(
      op.ComputeOutputShape({std::make_shared<TypeParam>(a), std::make_shared<TypeParam>(b)}));
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

TYPED_TEST(MatrixMultiplyTest, saveparams_test)
{
  using ArrayType     = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MatrixMultiply<ArrayType>::SPType;
  using OpType        = typename fetch::ml::ops::MatrixMultiply<ArrayType>;

  TypeParam data_1 = TypeParam::FromString("1, 2, -3, 4, 5");
  TypeParam data_2 = TypeParam::FromString(
      "-11, 12, 13, 14; 21, 22, 23, 24; 31, 32, 33, 34; 41, 42, 43, 44; 51, 52, 53, 54");
  TypeParam gt = TypeParam::FromString("357, 388, 397, 406");

  OpType op;

  ArrayType     prediction(op.ComputeOutputShape({data_1, data_2}));
  VecTensorType vec_data({data_1, data_2});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::ByteArrayBuffer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  ArrayType new_prediction(op.ComputeOutputShape({data_1, data_2}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                      fetch::math::function_tolerance<DataType>()));
}
