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
#include "math/tensor/tensor.hpp"
#include "ml/ops/top_k.hpp"
#include "ml/serializers/ml_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace tests {

template <typename T>
class TopKOpTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TopKOpTest, MyTypes);

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

TYPED_TEST(TopKOpTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = fetch::math::SizeType;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TopK<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::TopK<TensorType>;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  TensorType gt = TypeParam::FromString("13,14,15,16;9,10,11,12");
  gt.Reshape({2, 4});

  SizeType k      = 2;
  bool     sorted = true;

  OpType op(k, sorted);

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

TYPED_TEST(TopKOpTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::TopK<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  TensorType error = TypeParam::FromString("20,-21,22,-23;24,-25,26,-27");
  error.Reshape({2, 4});

  SizeType k      = 2;
  bool     sorted = true;

  fetch::ml::ops::TopK<TypeParam> op(k, sorted);

  // Run forward pass before backward pass
  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(data)}));
  op.Forward({std::make_shared<TypeParam>(data)}, prediction);

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

  // Run forward pass before backward pass
  new_op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

}  // namespace tests
}  // namespace ml
}  // namespace fetch
