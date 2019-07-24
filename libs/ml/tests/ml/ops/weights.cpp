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
#include "ml/ops/weights.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

#include <memory>

template <typename T>
class WeightsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(WeightsTest, MyTypes);

TYPED_TEST(WeightsTest, allocation_test)
{
  fetch::ml::ops::Weights<TypeParam> w;
}

TYPED_TEST(WeightsTest, gradient_step_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType        data(8);
  ArrayType        error(8);
  ArrayType        gt(8);
  std::vector<int> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({2, -4, 0, 1, 13, -19, 28, 26});
  for (SizeType i{0}; i < 8; ++i)
  {
    data.Set(i, static_cast<DataType>(dataInput[i]));
    error.Set(i, static_cast<DataType>(errorInput[i]));
    gt.Set(i, static_cast<DataType>(gtInput[i]));
  }

  fetch::ml::ops::Weights<ArrayType> w;
  w.SetData(data);

  ArrayType prediction(w.ComputeOutputShape({}));
  w.Forward({}, prediction);

  EXPECT_EQ(prediction, data);
  std::vector<ArrayType> error_signal = w.Backward({}, error);

  ArrayType grad = w.get_gradients();
  fetch::math::Multiply(grad, DataType{-1}, grad);
  w.ApplyGradient(grad);

  prediction = ArrayType(w.ComputeOutputShape({}));
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
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;
  using SPType    = typename fetch::ml::ops::Weights<ArrayType>::SPType;
  using OpType    = typename fetch::ml::ops::Weights<ArrayType>;

  ArrayType data = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  ArrayType gt   = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpType op;
  op.SetData(data);

  ArrayType prediction(op.ComputeOutputShape({data}));

  op.Forward({}, prediction);

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
  ArrayType new_prediction(op.ComputeOutputShape({data}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                      fetch::math::function_tolerance<DataType>()));
}
