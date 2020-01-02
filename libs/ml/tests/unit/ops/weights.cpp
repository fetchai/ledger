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
#include "ml/ops/weights.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class WeightsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(WeightsTest, math::test::TensorIntAndFloatingTypes);

TYPED_TEST(WeightsTest, allocation_test)
{
  fetch::ml::ops::Weights<TypeParam> w;
}

TYPED_TEST(WeightsTest, gradient_step_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;

  TensorType       data(8);
  TensorType       error(8);
  TensorType       gt(8);
  std::vector<int> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({2, -4, 0, 1, 13, -19, 28, 26});
  for (SizeType i{0}; i < 8; ++i)
  {
    data.Set(i, static_cast<DataType>(dataInput[i]));
    error.Set(i, static_cast<DataType>(errorInput[i]));
    gt.Set(i, static_cast<DataType>(gtInput[i]));
  }

  fetch::ml::ops::Weights<TensorType> w;
  w.SetData(data);

  TensorType prediction(w.ComputeOutputShape({}));
  w.Forward({}, prediction);

  EXPECT_EQ(prediction, data);
  std::vector<TensorType> error_signal = w.Backward({}, error);

  TensorType grad = w.GetGradientsReferences();
  fetch::math::Multiply(grad, DataType{-1}, grad);
  w.ApplyGradient(grad);

  prediction = TensorType(w.ComputeOutputShape({}));
  w.Forward({}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt));  // with new values
}

TYPED_TEST(WeightsTest, stateDict)
{
  fetch::ml::ops::Weights<TypeParam> w;
  fetch::ml::StateDict<TypeParam>    sd = w.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_TRUE(sd.dict_.empty());

  TypeParam data(8);
  w.SetData(data);
  sd = w.StateDict();
  EXPECT_EQ(*(sd.weights_), data);
  EXPECT_TRUE(sd.dict_.empty());
}

TYPED_TEST(WeightsTest, loadStateDict)
{
  fetch::ml::ops::Weights<TypeParam> w;

  std::shared_ptr<TypeParam>      data = std::make_shared<TypeParam>(8);
  fetch::ml::StateDict<TypeParam> sd;
  sd.weights_ = data;
  w.LoadStateDict(sd);

  TypeParam prediction(w.ComputeOutputShape({}));
  w.Forward({}, prediction);

  EXPECT_EQ(prediction, *data);
}

TYPED_TEST(WeightsTest, saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::Weights<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::Weights<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpType op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

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
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(WeightsTest, saveparams_gradient_step_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::Weights<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType       data(8);
  TensorType       error(8);
  std::vector<int> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  for (SizeType i{0}; i < 8; ++i)
  {
    data.Set(i, static_cast<DataType>(dataInput[i]));
    error.Set(i, static_cast<DataType>(errorInput[i]));
  }

  fetch::ml::ops::Weights<TensorType> op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  std::vector<TensorType> error_signal = op.Backward({}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  op.Backward({}, error);

  TensorType grad = op.GetGradientsReferences();
  fetch::math::Multiply(grad, DataType{-1}, grad);
  op.ApplyGradient(grad);

  prediction = TensorType(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  new_op.Backward({}, error);

  TensorType new_grad = new_op.GetGradientsReferences();
  fetch::math::Multiply(new_grad, DataType{-1}, new_grad);
  new_op.ApplyGradient(new_grad);

  TensorType new_prediction = TensorType(new_op.ComputeOutputShape({}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(new_prediction,
                                  fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
