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

#include "core/serializers/main_serializer.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"
#include "ml/serializers/ml_types.hpp"

template <typename T>
class SelfAttentionEncoder : public ::testing::Test
{
};

using Types = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                               fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                               fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(SelfAttentionEncoder, Types);

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

TYPED_TEST(SelfAttentionEncoder, saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;
  using LayerType = typename fetch::ml::layers::SelfAttentionEncoder<TypeParam>;
  using SPType    = typename LayerType::SPType;

  SizeType n_heads   = 2;
  SizeType model_dim = 6;
  SizeType ff_dim    = 12;

  std::string input_name  = "SelfAttentionEncoder_Input";
  std::string output_name = "SelfAttentionEncoder_Feedforward_Residual_LayerNorm";

  // create input
  TypeParam input({model_dim, 25, n_heads});
  input.FillUniformRandom();

  // create labels
  TypeParam labels({model_dim, 25, n_heads});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(n_heads, model_dim, ff_dim);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and ForwardPropagate
  layer.SetInput(input_name, input);
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

  // rebuild
  auto layer2 = *(fetch::ml::utilities::BuildLayer<TypeParam, LayerType>(dsp2));

  // test equality
  layer.SetInput(input_name, input);
  prediction = layer.Evaluate(output_name, true);
  layer2.SetInput(input_name, input);
  TypeParam prediction2 = layer2.Evaluate(output_name, true);

  ASSERT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TypeParam loss = layer.Evaluate(error_output);
  layer.BackPropagateError(error_output);
  layer.Step(DataType{0.1f});

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagateError(error_output);
  layer2.Step(DataType{0.1f});

  EXPECT_TRUE(loss.AllClose(loss2, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  // new random input
  input.FillUniformRandom();

  layer.SetInput(input_name, input);
  TypeParam prediction3 = layer.Evaluate(output_name);

  layer2.SetInput(input_name, input);
  TypeParam prediction4 = layer2.Evaluate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}
