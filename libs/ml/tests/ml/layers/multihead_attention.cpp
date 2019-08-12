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

#include "ml/layers/multihead_attention.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class MultiheadAttention : public ::testing::Test
{
};
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(MultiheadAttention, MyTypes);

TYPED_TEST(MultiheadAttention, input_output_dimension_check)  // Use the class as a subgraph
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  fetch::ml::Graph<TypeParam> g;

  std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
  std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
  std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
  g.template AddNode<fetch::ml::layers::MultiheadAttention<TypeParam>>(
      "ScaledDotProductAttention", {query, key, value}, static_cast<SizeType>(4),
      static_cast<SizeType>(12), DataType(0.1));
  TypeParam query_data = TypeParam({12, 25, 4});  // todo - values not initialised?
  TypeParam key_data   = query_data;
  TypeParam value_data = query_data;
  g.SetInput(query, query_data);
  g.SetInput(key, key_data);
  g.SetInput(value, value_data);

  TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 12);
  ASSERT_EQ(prediction.shape()[1], 25);
  ASSERT_EQ(prediction.shape()[2], 4);
}

TYPED_TEST(MultiheadAttention, backward_test)  // Use the class as an Ops
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;
  fetch::ml::layers::MultiheadAttention<TypeParam> m_att(static_cast<SizeType>(4),
                                                         static_cast<SizeType>(12), DataType(0.9));
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({12, 20, 5}));
  TypeParam output(m_att.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  m_att.Forward({std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(input_data),
                 std::make_shared<TypeParam>(input_data)},
                output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({12, 20, 5}));

  std::vector<TypeParam> backprop_error = m_att.Backward(
      {std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(input_data),
       std::make_shared<TypeParam>(input_data)},
      error_signal);

  // check there are proper number of error signals, this is an indirect test for subgraph backward
  // signal pass
  ASSERT_EQ(backprop_error.size(), 3);

  // check all shape are the same
  bool                  all_same_shape = true;
  std::vector<SizeType> prev_shape;
  for (auto error : backprop_error)
  {
    auto shape = error.shape();
    if (prev_shape.size() == 0)
    {
      prev_shape = shape;
      continue;
    }
    else
    {
      if (shape != prev_shape)
      {
        all_same_shape = false;
        break;
      }
      else
      {
        prev_shape = shape;
      }
    }
  }
  ASSERT_TRUE(all_same_shape);

  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 12);
  ASSERT_EQ(backprop_error[0].shape()[1], 20);
  ASSERT_EQ(backprop_error[0].shape()[2], 5);
}

TYPED_TEST(MultiheadAttention, saveparams_test)
{
  using DataType = typename TypeParam::Type;

  fetch::math::SizeType n_heads   = 3;
  fetch::math::SizeType model_dim = 6;

  TypeParam query_data = TypeParam({model_dim, 12, n_heads});
  TypeParam key_data   = query_data;
  TypeParam value_data = query_data;

  auto mha_layer =
      std::make_shared<fetch::ml::layers::MultiheadAttention<TypeParam>>(n_heads, model_dim);

  mha_layer->SetInput("MultiheadAttention_Query", query_data);
  mha_layer->SetInput("MultiheadAttention_Key", key_data);
  mha_layer->SetInput("MultiheadAttention_Value", value_data);

  auto output = mha_layer->Evaluate("MultiheadAttention_Final_Transformation", true);

  // extract saveparams
  auto sp = mha_layer->GetOpSaveableParams();

  // downcast to correct type
  auto dsp =
      std::dynamic_pointer_cast<typename fetch::ml::layers::MultiheadAttention<TypeParam>::SPType>(
          sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<typename fetch::ml::layers::MultiheadAttention<TypeParam>::SPType>();
  b >> *dsp2;

  // rebuild
  auto sa2 =
      fetch::ml::utilities::BuildLayer<TypeParam, fetch::ml::layers::MultiheadAttention<TypeParam>>(
          dsp2);

  sa2->SetInput("MultiheadAttention_Query", query_data);
  sa2->SetInput("MultiheadAttention_Key", key_data);
  sa2->SetInput("MultiheadAttention_Value", value_data);

  TypeParam output2 = sa2->Evaluate("MultiheadAttention_Final_Transformation", true);

  ASSERT_TRUE(output.AllClose(output2, static_cast<DataType>(0), static_cast<DataType>(0)));
}

TYPED_TEST(MultiheadAttention, saveparams_test2)
{
  using DataType  = typename TypeParam::Type;
  using LayerType = typename fetch::ml::layers::MultiheadAttention<TypeParam>;
  using SPType    = typename LayerType::SPType;

  fetch::math::SizeType n_heads   = 3;
  fetch::math::SizeType model_dim = 6;

  std::string output_name = "MultiheadAttention_Final_Transformation";

  // create input data
  TypeParam query_data = TypeParam({model_dim, 12, n_heads});
  query_data.FillUniformRandom();

  TypeParam key_data   = query_data;
  TypeParam value_data = query_data;

  // create labels data
  TypeParam labels({6, 12, 3});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(n_heads, model_dim);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and evaluate
  layer.SetInput("MultiheadAttention_Query", query_data);
  layer.SetInput("MultiheadAttention_Key", key_data);
  layer.SetInput("MultiheadAttention_Value", value_data);

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
  layer.SetInput("MultiheadAttention_Query", query_data);
  layer.SetInput("MultiheadAttention_Key", key_data);
  layer.SetInput("MultiheadAttention_Value", value_data);
  prediction = layer.Evaluate(output_name, true);

  layer2.SetInput("MultiheadAttention_Query", query_data);
  layer2.SetInput("MultiheadAttention_Key", key_data);
  layer2.SetInput("MultiheadAttention_Value", value_data);
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
  query_data.FillUniformRandom();

  layer.SetInput("MultiheadAttention_Query", query_data);
  layer.SetInput("MultiheadAttention_Key", key_data);
  layer.SetInput("MultiheadAttention_Value", value_data);
  TypeParam prediction3 = layer.Evaluate(output_name);

  layer2.SetInput("MultiheadAttention_Query", query_data);
  layer2.SetInput("MultiheadAttention_Key", key_data);
  layer2.SetInput("MultiheadAttention_Value", value_data);
  TypeParam prediction4 = layer2.Evaluate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}
