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

#include "gtest/gtest.h"
#include "math/tensor.hpp"
#include "ml/layers/self_attention.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

template <typename T>
class SelfAttentionTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(SelfAttentionTest, MyTypes);

TYPED_TEST(SelfAttentionTest, output_shape_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::SelfAttention<TypeParam>>("SelfAttention", {"Input"}, 50u,
                                                                  42u, 10u);

  TypeParam data({5, 10, 1});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("SelfAttention", true);
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 1);
}

TYPED_TEST(SelfAttentionTest, saveparams_test)
{
  using DataType = typename TypeParam::Type;

  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  fetch::math::SizeType in_size     = 50u;
  fetch::math::SizeType out_size    = 42u;
  fetch::math::SizeType hidden_size = 10u;

  auto sa_layer = std::make_shared<fetch::ml::layers::SelfAttention<TypeParam>>(
      in_size, out_size, hidden_size, "SelfAttention");

  sa_layer->SetInput("SelfAttention_Input", data);

  TypeParam output = sa_layer->Evaluate("SelfAttention_OutputFC", true);

  // extract saveparams
  auto sp = sa_layer->GetOpSaveableParams();

  // downcast to correct type
  auto dsp =
      std::dynamic_pointer_cast<typename fetch::ml::layers::SelfAttention<TypeParam>::SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<typename fetch::ml::layers::SelfAttention<TypeParam>::SPType>();
  b >> *dsp2;

  // rebuild
  auto sa2 = fetch::ml::utilities::BuildLayerSelfAttention<TypeParam>(*dsp2);

  sa2->SetInput("SelfAttention_Input", data);
  TypeParam output2 = sa2->Evaluate("SelfAttention_OutputFC", true);

  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}
