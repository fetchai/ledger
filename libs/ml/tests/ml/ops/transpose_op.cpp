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
#include "ml/ops/transpose.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

#include <vector>

template <typename T>
class TransposeTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TransposeTest, MyTypes);

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

TYPED_TEST(TransposeTest, saveparams_test)
{
  using ArrayType     = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<ArrayType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Transpose<ArrayType>::SPType;
  using OpType        = typename fetch::ml::ops::Transpose<ArrayType>;

  ArrayType data = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");
  ArrayType gt   = TypeParam::FromString(R"(1, 4; 2, 5; -3, 6)");

  OpType op;

  ArrayType     prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(data)}));
  VecTensorType vec_data({std::make_shared<const ArrayType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParamsInterface> sp = op.GetOpSaveableParams();

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
  ArrayType new_prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                      fetch::math::function_tolerance<DataType>()));
}
