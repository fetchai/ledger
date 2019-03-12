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

#include "ml/ops/weights.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

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
  std::shared_ptr<TypeParam> data  = std::make_shared<TypeParam>(8);
  std::shared_ptr<TypeParam> error = std::make_shared<TypeParam>(8);
  std::shared_ptr<TypeParam> gt    = std::make_shared<TypeParam>(8);
  std::vector<int>           dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int>           errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int>           gtInput({2, -4, 0, 1, 13, -19, 28, 26});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data->Set(i, typename TypeParam::Type(dataInput[i]));
    error->Set(i, typename TypeParam::Type(errorInput[i]));
    gt->Set(i, typename TypeParam::Type(gtInput[i]));
  }

  fetch::ml::ops::Weights<TypeParam> w;
  w.SetData(data);
  ASSERT_EQ(w.Forward({}), *data);
  std::vector<TypeParam> errorSignal = w.Backward({}, *error);
  w.Step(typename TypeParam::Type(1));
  ASSERT_TRUE(w.Forward({}).AllClose(*gt));  // whit new values
}

TYPED_TEST(WeightsTest, stateDict)
{
  fetch::ml::ops::Weights<TypeParam>   w;
  fetch::ml::ops::StateDict<TypeParam> sd = w.StateDict();

  EXPECT_TRUE(sd.weights_ == nullptr);
  EXPECT_TRUE(sd.dict_.empty());

  std::shared_ptr<TypeParam> data = std::make_shared<TypeParam>(8);
  w.SetData(data);
  sd = w.StateDict();
  EXPECT_TRUE(sd.weights_ == data);
  EXPECT_TRUE(sd.dict_.empty());
}

TYPED_TEST(WeightsTest, loadStateDict)
{
  fetch::ml::ops::Weights<TypeParam> w;

  std::shared_ptr<TypeParam>           data = std::make_shared<TypeParam>(8);
  fetch::ml::ops::StateDict<TypeParam> sd;
  sd.weights_ = data;
  w.LoadStateDict(sd);
  EXPECT_EQ(w.Forward({}), *data);
}
