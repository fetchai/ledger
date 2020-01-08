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
#include "ml/ops/concatenate.hpp"
#include "test_types.hpp"

#include "core/serializers/main_serializer_definition.hpp"
#include "gtest/gtest.h"
#include "ml/serializers/ml_types.hpp"
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class ConcatenateTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ConcatenateTest, math::test::TensorIntAndFloatingTypes);

TYPED_TEST(ConcatenateTest, forward_test)
{
  TypeParam data1(std::vector<fetch::math::SizeType>({8, 8}));
  TypeParam data2(std::vector<fetch::math::SizeType>({8, 8}));

  fetch::ml::ops::Concatenate<TypeParam> op{1};

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}));
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, prediction);

  // test correct shape
  ASSERT_EQ(prediction.shape(), std::vector<typename TypeParam::SizeType>({8, 16}));
}

TYPED_TEST(ConcatenateTest, compute_output_shape_test)
{
  TypeParam data1(std::vector<fetch::math::SizeType>({8, 8, 10}));
  TypeParam data2(std::vector<fetch::math::SizeType>({8, 8, 2}));

  fetch::ml::ops::Concatenate<TypeParam> op{2};

  std::vector<typename TypeParam::SizeType> output_shape = op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)});

  // test correct computed output shape
  ASSERT_EQ(output_shape, std::vector<typename TypeParam::SizeType>({8, 8, 12}));
}

TYPED_TEST(ConcatenateTest, backward_test)
{
  TypeParam data1(std::vector<fetch::math::SizeType>({8, 8}));
  TypeParam data2(std::vector<fetch::math::SizeType>({8, 8}));

  fetch::ml::ops::Concatenate<TypeParam> op{1};

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}));
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, prediction);

  TypeParam              error_signal(prediction.shape());
  std::vector<TypeParam> gradients = op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  ASSERT_EQ(gradients.size(), 2);
  ASSERT_EQ(gradients[0].shape(), std::vector<typename TypeParam::SizeType>({8, 8}));
  ASSERT_EQ(gradients[1].shape(), std::vector<typename TypeParam::SizeType>({8, 8}));
}

TYPED_TEST(ConcatenateTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Concatenate<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Concatenate<TensorType>;

  TypeParam data1 = TypeParam::UniformRandom(64);
  TypeParam data2 = TypeParam::UniformRandom(64);
  data1.Reshape({8, 8});
  data2.Reshape({8, 8});

  OpType op(1);

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data1), std::make_shared<const TensorType>(data2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data1), std::make_shared<const TensorType>(data2)});

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
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data1), std::make_shared<const TensorType>(data2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(ConcatenateTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::Concatenate<TensorType>;
  using SPType     = typename OpType::SPType;

  TypeParam data1(std::vector<fetch::math::SizeType>({8, 8}));
  TypeParam data2(std::vector<fetch::math::SizeType>({8, 8}));

  fetch::ml::ops::Concatenate<TypeParam> op{1};

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}));
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, prediction);

  TypeParam              error_signal(prediction.shape());
  std::vector<TypeParam> gradients = op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  gradients = op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal);
  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_gradients = new_op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  // test correct values
  EXPECT_TRUE(gradients.at(0).AllClose(
      new_gradients.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(gradients.at(1).AllClose(
      new_gradients.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
