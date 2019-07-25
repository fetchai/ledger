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

#include "math/tensor.hpp"
#include "ml/ops/concatenate.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class ConcatenateTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(ConcatenateTest, MyTypes);

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
