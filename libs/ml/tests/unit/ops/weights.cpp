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
#include "ml/ops/weights.hpp"
#include "ml/state_dict.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using namespace fetch::ml;

template <typename T>
class WeightsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(WeightsTest, fetch::math::test::TensorIntAndFloatingTypes);

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

}  // namespace
