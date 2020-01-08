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

#include "core/serializers/main_serializer.hpp"
#include "gtest/gtest.h"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class SelfAttentionEncoder : public ::testing::Test
{
};

TYPED_TEST_CASE(SelfAttentionEncoder, math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(SelfAttentionEncoder, input_output_dimension_test)  // Use the class as a part of a graph
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  fetch::ml::Graph<TypeParam> g;

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  std::string mask  = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Mask", {});

  std::string output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<TypeParam>>(
      "SelfAttentionEncoder", {input, mask}, static_cast<SizeType>(4), static_cast<SizeType>(12),
      static_cast<SizeType>(24));
  TypeParam input_data = TypeParam({12, 25, 4});
  input_data.Fill(fetch::math::Type<DataType>("0.01"));
  TypeParam mask_data = TypeParam({25, 25, 4});
  mask_data.Fill(DataType{1});
  g.SetInput(input, input_data);
  g.SetInput(mask, mask_data);

  TypeParam prediction = g.Evaluate(output, false);
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 12);
  ASSERT_EQ(prediction.shape()[1], 25);
  ASSERT_EQ(prediction.shape()[2], 4);
}

TYPED_TEST(SelfAttentionEncoder, backward_dimension_test)  // Use the class as a subgraph
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;
  fetch::ml::layers::SelfAttentionEncoder<TypeParam> encoder(
      static_cast<SizeType>(4), static_cast<SizeType>(12), static_cast<SizeType>(13));
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({12, 20, 5}));
  input_data.Fill(fetch::math::Type<DataType>("0.1"));
  TypeParam mask_data = TypeParam({20, 20, 5});
  mask_data.Fill(DataType{1});

  TypeParam output(encoder.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  encoder.Forward({std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(mask_data)},
                  output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({12, 20, 5}));

  std::vector<TypeParam> backprop_error = encoder.Backward(
      {std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(mask_data)},
      error_signal);

  // check there are proper number of error signals
  ASSERT_EQ(backprop_error.size(), 2);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 12);
  ASSERT_EQ(backprop_error[0].shape()[1], 20);
  ASSERT_EQ(backprop_error[0].shape()[2], 5);

  ASSERT_EQ(backprop_error[1].shape().size(), 3);
  ASSERT_EQ(backprop_error[1].shape()[0], 20);
  ASSERT_EQ(backprop_error[1].shape()[1], 20);
  ASSERT_EQ(backprop_error[1].shape()[2], 5);
}

TYPED_TEST(SelfAttentionEncoder, saveparams_test)
{
  using SizeType  = fetch::math::SizeType;
  using LayerType = typename fetch::ml::layers::SelfAttentionEncoder<TypeParam>;
  using SPType    = typename LayerType::SPType;
  using DataType  = typename TypeParam::Type;

  SizeType n_heads   = 2;
  SizeType model_dim = 6;
  SizeType ff_dim    = 12;

  std::string input_name  = "SelfAttentionEncoder_Input";
  std::string mask_name   = "SelfAttentionEncoder_Mask";
  std::string output_name = "SelfAttentionEncoder_Feedforward_Residual_LayerNorm";

  // create input
  TypeParam input({model_dim, 25, 2});
  input.FillUniformRandom();

  TypeParam mask_data = TypeParam({25, 25, 2});
  mask_data.Fill(DataType{1});

  // create labels
  TypeParam labels({model_dim, 25, 2});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(n_heads, model_dim, ff_dim);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and evaluate
  layer.SetInput(input_name, input);
  layer.SetInput(mask_name, mask_data);
  TypeParam prediction;
  prediction = layer.Evaluate(output_name, true);

  // extract saveparams
  auto sp = layer.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
