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
#include "ml/ops/transpose.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class TransposeTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TransposeTest, math::test::TensorIntAndFloatingTypes);

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
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Transpose<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Transpose<TensorType>;

  TensorType data = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");
  TensorType gt   = TypeParam::FromString(R"(1, 4; 2, 5; -3, 6)");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(TransposeTest, saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Transpose<TensorType>;
  using SPType     = typename OpType::SPType;
  TypeParam a({4, 5, 2});
  TypeParam error({5, 4, 2});

  fetch::ml::ops::Transpose<TypeParam> op;
  std::vector<TypeParam>               backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals = op.Backward({std::make_shared<TypeParam>(a)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
