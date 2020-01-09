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

#include "ml/ops/slice.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SliceTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SliceTest, math::test::TensorIntAndFloatingTypes);

TYPED_TEST(SliceTest, multi_axes_forward_shape_test)
{
  using SizeVector = typename TypeParam::SizeVector;

  TypeParam  a({1, 2, 3, 4, 5});
  SizeVector axes({3, 4});
  SizeVector indices({3, 4});
  TypeParam  gt({1, 2, 3, 1, 1});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(SliceTest, single_axis_forward_shape_test)
{
  using SizeVector = typename TypeParam::SizeVector;

  TypeParam  a({1, 2, 3, 4, 5});
  SizeVector axes({3});
  SizeVector indices({3});
  TypeParam  gt({1, 2, 3, 1, 5});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(SliceTest, single_axis_forward_2D_value_test)
{
  using SizeType = fetch::math::SizeType;

  TypeParam a = TypeParam::FromString("1, 2, 3; 4, 5, 6");
  SizeType  axis(0u);
  SizeType  index(1u);
  TypeParam gt = TypeParam::FromString("4, 5, 6");
  gt.Reshape({1, 3});

  fetch::ml::ops::Slice<TypeParam> op(index, axis);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(SliceTest, single_axis_forward_3D_value_test)
{
  using SizeType = fetch::math::SizeType;

  TypeParam a = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  a.Reshape({3, 2, 2});
  SizeType  axis(1u);
  SizeType  index(1u);
  TypeParam gt = TypeParam::FromString("2, 4; 5, 7; -2, -4");
  gt.Reshape({3, 1, 2});

  fetch::ml::ops::Slice<TypeParam> op(index, axis);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(SliceTest, multi_axes_forward_3D_value_test)
{
  using SizeVector = typename TypeParam::SizeVector;

  TypeParam a = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  a.Reshape({3, 2, 2});
  SizeVector axes({1, 2});
  SizeVector indices({1, 1});
  TypeParam  gt = TypeParam::FromString("4; 7; -4");
  gt.Reshape({3, 1, 1});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(SliceTest, single_axis_backward_3D_value_test)
{
  using SizeType = fetch::math::SizeType;

  TypeParam a = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  a.Reshape({3, 2, 2});
  SizeType axis(1u);
  SizeType index(0u);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3");
  error.Reshape({3, 1, 2});
  TypeParam gt = TypeParam::FromString("1, 0, 3, 0; 4, 0, 6, 0; -1, 0, -3, 0");
  gt.Reshape({3, 2, 2});

  fetch::ml::ops::Slice<TypeParam> op(index, axis);
  // run backward twice to make sure the buffering is working
  op.Backward({std::make_shared<TypeParam>(a)}, error);
  std::vector<TypeParam> backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), a.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gt));
}

TYPED_TEST(SliceTest, ranged_forward_shape_test)
{
  using SizeType     = fetch::math::SizeType;
  using SizePairType = std::pair<SizeType, SizeType>;

  TypeParam    a({5, 30, 6});
  SizeType     axis{1};
  SizePairType start_end_slice = std::make_pair(10, 20);
  TypeParam    gt({5, 10, 6});

  fetch::ml::ops::Slice<TypeParam> op(start_end_slice, axis);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));
  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  // test correct values
  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(SliceTest, ranged_forward_3D_value_test)
{
  using SizeType     = fetch::math::SizeType;
  using SizePairType = std::pair<SizeType, SizeType>;

  TypeParam a = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  a.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  TypeParam gt = TypeParam::FromString("4, 6;-1, -3;5, 7;-2, -4");
  gt.Reshape({2, 2, 2});

  fetch::ml::ops::Slice<TypeParam> op(start_end_slice, axis);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(a)}));

  op.Forward({std::make_shared<TypeParam>(a)}, prediction);

  ASSERT_EQ(prediction.shape(), gt.shape());
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(SliceTest, ranged_backward_3D_value_test)
{
  using SizeType     = fetch::math::SizeType;
  using SizePairType = std::pair<SizeType, SizeType>;

  TypeParam a = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  a.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3; -2, -3");
  error.Reshape({2, 2, 2});
  TypeParam gt = TypeParam::FromString("0, 0, 0, 0; 1, -1, 3, -3; 4, -2, 6, -3");
  gt.Reshape({3, 2, 2});

  fetch::ml::ops::Slice<TypeParam> op(start_end_slice, axis);

  // run backward twice to make sure the buffering is working
  op.Backward({std::make_shared<TypeParam>(a)}, error);
  std::vector<TypeParam> backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct shapes
  ASSERT_EQ(backpropagated_signals.size(), 1);
  ASSERT_EQ(backpropagated_signals[0].shape(), a.shape());

  // test correct values
  EXPECT_TRUE(backpropagated_signals[0].AllClose(gt));
}

TYPED_TEST(SliceTest, single_axis_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeVector    = typename TypeParam::SizeVector;

  TypeParam  data({1, 2, 3, 4, 5});
  SizeVector axes({3});
  SizeVector indices({3});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

  // forward pass
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
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SliceTest, single_axis_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Slice<TensorType>;
  using SPType     = typename OpType::SPType;
  using SizeType   = fetch::math::SizeType;

  TypeParam data = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  data.Reshape({3, 2, 2});
  SizeType axis(1u);
  SizeType index(0u);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3");
  error.Reshape({3, 1, 2});

  fetch::ml::ops::Slice<TypeParam>               op(index, axis);
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SliceTest, ranged_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeType      = fetch::math::SizeType;
  using SizePairType  = std::pair<SizeType, SizeType>;

  TypeParam data = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  data.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  fetch::ml::ops::Slice<TypeParam> op(start_end_slice, axis);

  // forward pass
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
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SliceTest, ranged_saveparams_backward_test)
{
  using TensorType   = TypeParam;
  using DataType     = typename TypeParam::Type;
  using OpType       = fetch::ml::ops::Slice<TensorType>;
  using SPType       = typename OpType::SPType;
  using SizeType     = fetch::math::SizeType;
  using SizePairType = std::pair<SizeType, SizeType>;

  TypeParam data = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  data.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3; -2, -3");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::Slice<TypeParam>               op(start_end_slice, axis);
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SliceTest, multi_axes_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeVector    = typename TypeParam::SizeVector;

  TypeParam data = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  data.Reshape({3, 2, 2});
  SizeVector axes({1, 2});
  SizeVector indices({1, 1});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

  // forward pass
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
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
