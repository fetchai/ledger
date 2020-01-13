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

#include "gtest/gtest.h"
#include <memory>
#include <vector>

namespace {

template <typename T>
class ConcatenateTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ConcatenateTest, fetch::math::test::TensorIntAndFloatingTypes);

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

}  // namespace
