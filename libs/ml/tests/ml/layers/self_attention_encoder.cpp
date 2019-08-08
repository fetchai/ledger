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

#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <ml/layers/self_attention_encoder.hpp>

template <typename T>
class SelfAttentionEncoder : public ::testing::Test
{
};
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(SelfAttentionEncoder, MyTypes);

TYPED_TEST(SelfAttentionEncoder, input_output_dimension_test)  // Use the class as a part of a graph
{
  using SizeType = typename TypeParam::SizeType;

  fetch::ml::Graph<TypeParam> g;

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});

  std::string output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<TypeParam>>(
      "ScaledDotProductAttention", {input}, static_cast<SizeType>(4), static_cast<SizeType>(12),
      static_cast<SizeType>(24));
  TypeParam input_data = TypeParam({12, 25, 4});
  g.SetInput(input, input_data);

  TypeParam prediction = g.Evaluate(output, false);
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 12);
  ASSERT_EQ(prediction.shape()[1], 25);
  ASSERT_EQ(prediction.shape()[2], 4);
}

TYPED_TEST(SelfAttentionEncoder, backward_dimension_test)  // Use the class as a subgraph
{
  using SizeType = typename TypeParam::SizeType;
  fetch::ml::layers::SelfAttentionEncoder<TypeParam> encoder(
      static_cast<SizeType>(4), static_cast<SizeType>(12), static_cast<SizeType>(13));
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({12, 20, 5}));
  TypeParam output(encoder.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  encoder.Forward({std::make_shared<TypeParam>(input_data)}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({12, 20, 5}));

  std::vector<TypeParam> backprop_error =
      encoder.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);

  // check there are proper number of error signals
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 12);
  ASSERT_EQ(backprop_error[0].shape()[1], 20);
  ASSERT_EQ(backprop_error[0].shape()[2], 5);
}
