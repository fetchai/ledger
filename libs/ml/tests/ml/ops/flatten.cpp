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

#include "ml/ops/flatten.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <gtest/gtest.h>

template <typename T>
class FlattenTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(FlattenTest, MyTypes);

TYPED_TEST(FlattenTest, forward_test)
{
  TypeParam                          data(std::vector<std::uint64_t>({8, 8}));
  fetch::ml::ops::Flatten<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct shape
  ASSERT_EQ(prediction.shape(), std::vector<typename TypeParam::SizeType>({1, 64}));
}

TYPED_TEST(FlattenTest, backward_test)
{
  TypeParam                          data(std::vector<std::uint64_t>({8, 8}));
  fetch::ml::ops::Flatten<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  TypeParam              error_signal(prediction.shape());
  std::vector<TypeParam> gradients = op.Backward({data}, error_signal);

  ASSERT_EQ(gradients.size(), 1);
  ASSERT_EQ(gradients[0].shape(), std::vector<typename TypeParam::SizeType>({8, 8}));
}
