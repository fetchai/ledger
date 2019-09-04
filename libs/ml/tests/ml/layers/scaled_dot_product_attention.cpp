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
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"

#include "ml/regularisers/regulariser.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class ScaledDotProductAttention : public ::testing::Test
{
};
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(ScaledDotProductAttention, MyTypes);

TYPED_TEST(ScaledDotProductAttention, input_output_dimension_check)  // Use the class as a subgraph
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  fetch::ml::Graph<TypeParam> g;

  std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
  std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
  std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
  std::string mask  = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Mask", {});
  g.template AddNode<fetch::ml::layers::ScaledDotProductAttention<TypeParam>>(
      "ScaledDotProductAttention", {query, key, value, mask}, static_cast<SizeType>(4),
      DataType(0.1));
  TypeParam query_data = TypeParam({4, 7, 2});
  TypeParam key_data   = TypeParam({4, 5, 2});
  TypeParam value_data = TypeParam({3, 5, 2});
  TypeParam mask_data  = TypeParam({1, 7, 2});
  g.SetInput(query, query_data);
  g.SetInput(key, key_data);
  g.SetInput(value, value_data);
  g.SetInput(mask, mask_data);

  TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
  ASSERT_EQ(prediction.shape()[0], 3);
  ASSERT_EQ(prediction.shape()[1], 7);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(ScaledDotProductAttention,
           self_attention_output_value_test)  // Use the class as a subgraph
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  fetch::ml::Graph<TypeParam> g;

  std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
  std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
  std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
  std::string mask  = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Mask", {});
  g.template AddNode<fetch::ml::layers::ScaledDotProductAttention<TypeParam>>(
      "ScaledDotProductAttention", {query, key, value, mask}, static_cast<SizeType>(3),
      DataType(0.1));
  TypeParam query_data = TypeParam::FromString("1, 2, 0.5, 0.1; 2, 1, 0.3, -0.2;2, 4, 0, 1");
  query_data.Reshape({3, 2, 2});

  // create sudo mask
  TypeParam mask_data = TypeParam({1, 2, 2});
  mask_data.Fill(static_cast<DataType>(1));

  g.SetInput(query, query_data);
  g.SetInput(key, query_data);
  g.SetInput(value, query_data);
  g.SetInput(mask, mask_data);

  TypeParam gt = TypeParam::FromString(
      "1.8496745531, 1.9944926680, 0.3201387782, 0.2406420371; 1.1503254469, 1.0055073320, "
      "0.0751734728, -0.0241974536; 3.6993491062, 3.9889853359, 0.4496530544, 0.6483949073");
  gt.Reshape({3, 2, 2});
  TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);

  ASSERT_TRUE(prediction.AllClose(
      gt, static_cast<DataType>(5) * fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ScaledDotProductAttention,
           self_attention_backward_exact_value_test)  // Use the class as a layer
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  fetch::ml::layers::ScaledDotProductAttention<TypeParam> att(static_cast<SizeType>(3),
                                                              DataType(1));

  TypeParam query_data = TypeParam::FromString("1, 2, 0.5, 0.1; 2, 1, 0.3, -0.2;2, 4, 0, 1");
  query_data.Reshape({3, 2, 2});

  // create sudo mask
  TypeParam mask_data = TypeParam({1, 2, 2});
  mask_data.Fill(static_cast<DataType>(1));

  TypeParam error_signal = TypeParam::FromString("1, 1, 0.2, -1.5; 1, 3, -0.3, 4; 1, 2.5, 7, 0");
  error_signal.Reshape({3, 2, 2});

  TypeParam gt_query_grad = TypeParam::FromString(
      "0.1474872519,  0.0094864446, -0.4040479300,  0.0737092770; -0.1474872519, -0.0094864446, "
      "-0.5050599125,  0.0921365963; 0.2949745039,  0.0189728892,  1.0101198249, -0.1842731926");
  gt_query_grad.Reshape({3, 2, 2});
  TypeParam gt_key_grad = TypeParam::FromString(
      "-0.1664601411,  0.1664601411, -0.4866325932,  0.4866325932; -0.3044609485,  0.3044609485, "
      "-0.3398905860,  0.3398905860; -0.3329202822,  0.3329202822,  0.1842731926, -0.1842731926");
  gt_key_grad.Reshape({3, 2, 2});
  TypeParam gt_value_grad = TypeParam::FromString(
      "0.1558327790,  1.8441672210, -0.4173382500, -0.8826617500; 0.1668474430,  3.8331525570,  "
      "1.2413162873,  2.4586837127; 0.1640937770,  3.3359062230,  3.8524286190,  3.1475713810");
  gt_value_grad.Reshape({3, 2, 2});
  TypeParam gt_mask_grad({1, 2, 2});

  // do the forward
  TypeParam output(att.ComputeOutputShape(
      {std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(query_data),
       std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(mask_data)}));
  att.Forward({std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(query_data),
               std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(mask_data)},
              output);

  // do the backprop
  std::vector<TypeParam> backprop_error = att.Backward(
      {std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(query_data),
       std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(mask_data)},
      error_signal);

  ASSERT_TRUE(backprop_error[0].AllClose(
      gt_query_grad, fetch::math::function_tolerance<DataType>(),
      static_cast<DataType>(10) * fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(backprop_error[1].AllClose(
      gt_key_grad, fetch::math::function_tolerance<DataType>(),
      static_cast<DataType>(10) * fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(backprop_error[2].AllClose(
      gt_value_grad, fetch::math::function_tolerance<DataType>(),
      static_cast<DataType>(10) * fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(backprop_error[3].AllClose(gt_mask_grad));
}

TYPED_TEST(ScaledDotProductAttention,
           self_attention_output_value_test_with_mask)  // Use the class as a subgraph
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  fetch::ml::Graph<TypeParam> g;

  std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
  std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
  std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
  std::string mask  = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Mask", {});
  g.template AddNode<fetch::ml::layers::ScaledDotProductAttention<TypeParam>>(
      "ScaledDotProductAttention", {query, key, value, mask}, static_cast<SizeType>(3),
      DataType(0.1));
  TypeParam query_data =
      TypeParam::FromString("1, 2, 0.5, 0.1, 5, 3; 2, 1, 0.3, -0.2, -2, 0.5; 2, 4, 0, 1, 1.1, -3");
  query_data.Reshape({3, 3, 2});

  // create mask
  TypeParam mask_data_one = TypeParam::FromString("1, 1; 1, 0; 0, 0");
  mask_data_one.Reshape({3, 1, 2});
  TypeParam mask_data({3, 3, 2});

  for (SizeType i{0}; i < mask_data.shape(1); i++)
  {
    mask_data.Slice(i, 1).Assign(mask_data_one);
  }

  g.SetInput(query, query_data);
  g.SetInput(key, query_data);
  g.SetInput(value, query_data);
  g.SetInput(mask, mask_data);

  TypeParam gt = TypeParam::FromString(
      "1.8496745531,  1.9944926680,  1.5288354812,  0.1000000000, 0.1000000000,  0.1000000000; "
      "1.1503254469,  1.0055073320,  1.4711645188, -0.2000000000, -0.2000000000, -0.2000000000; "
      "3.6993491062,  3.9889853359,  3.0576709623,  1.0000000000, 1.0000000000,  1.0000000000");
  gt.Reshape({3, 3, 2});
  TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
  std::cout << "pred: " << prediction.View(0).Copy().ToString() << std::endl;
  ASSERT_TRUE(prediction.AllClose(
      gt, static_cast<DataType>(5) * fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ScaledDotProductAttention,
           self_attention_backward_exact_value_test_with_mask)  // Use the class as a layer
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  fetch::ml::layers::ScaledDotProductAttention<TypeParam> att(static_cast<SizeType>(3),
                                                              DataType(1));

  TypeParam query_data =
      TypeParam::FromString("1, 2, 0.5, 0.1, 5, 3; 2, 1, 0.3, -0.2, -2, 0.5; 2, 4, 0, 1, 1.1, -3");
  query_data.Reshape({3, 3, 2});

  // create sudo mask
  TypeParam mask_data_one = TypeParam::FromString("1, 1; 1, 0; 0, 0");
  mask_data_one.Reshape({3, 1, 2});
  TypeParam mask_data({3, 3, 2});

  for (SizeType i{0}; i < mask_data.shape(1); i++)
  {
    mask_data.Slice(i, 1).Assign(mask_data_one);
  }

  TypeParam error_signal =
      TypeParam::FromString("1, 1, 0, -1.5, 0, 0; 1, 3, 0, 4, 0, 0; 1, 2.5, 0, 0, 0, 0");
  error_signal.Reshape({3, 3, 2});

  TypeParam gt_query_grad = TypeParam::FromString(
      "0.1474872519,  0.0094864446,  0.0000000000,  0.0000000000, 0.0000000000,  0.0000000000; "
      "-0.1474872519, -0.0094864446,  0.0000000000,  0.0000000000, 0.0000000000,  0.0000000000; "
      "0.2949745039,  0.0189728892,  0.0000000000,  0.0000000000,0.0000000000,  0.0000000000");
  gt_query_grad.Reshape({3, 3, 2});
  TypeParam gt_key_grad = TypeParam::FromString(
      "-0.1664601411,  0.1664601411,  0.0000000000,  0.0000000000,0.0000000000,  0.0000000000; "
      "-0.3044609485,  0.3044609485,  0.0000000000,  0.0000000000, 0.0000000000,  0.0000000000; "
      "-0.3329202822,  0.3329202822,  0.0000000000,  0.0000000000, 0.0000000000,  0.0000000000");
  gt_key_grad.Reshape({3, 3, 2});
  TypeParam gt_value_grad = TypeParam::FromString(
      "0.1558327790,  1.8441672210,  0.0000000000, -1.5000000000, 0.0000000000,  0.0000000000; "
      "0.1668474430,  3.8331525570,  0.0000000000,  4.0000000000, 0.0000000000,  0.0000000000; "
      "0.1640937770,  3.3359062230,  0.0000000000,  0.0000000000, 0.0000000000,  0.0000000000");
  gt_value_grad.Reshape({3, 3, 2});
  TypeParam gt_mask_grad({3, 3, 2});

  // do the forward
  TypeParam output(att.ComputeOutputShape(
      {std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(query_data),
       std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(mask_data)}));
  att.Forward({std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(query_data),
               std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(mask_data)},
              output);

  // do the backprop
  std::vector<TypeParam> backprop_error = att.Backward(
      {std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(query_data),
       std::make_shared<TypeParam>(query_data), std::make_shared<TypeParam>(mask_data)},
      error_signal);

  ASSERT_TRUE(backprop_error[0].AllClose(
      gt_query_grad, fetch::math::function_tolerance<DataType>(),
      static_cast<DataType>(10) * fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(backprop_error[1].AllClose(
      gt_key_grad, fetch::math::function_tolerance<DataType>(),
      static_cast<DataType>(10) * fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(backprop_error[2].AllClose(
      gt_value_grad, fetch::math::function_tolerance<DataType>(),
      static_cast<DataType>(10) * fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(backprop_error[3].AllClose(gt_mask_grad));
}

TYPED_TEST(ScaledDotProductAttention, saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;
  using LayerType = typename fetch::ml::layers::ScaledDotProductAttention<TypeParam>;
  using SPType    = typename LayerType::SPType;

  std::string output_name = "ScaledDotProductAttention_Value_Weight_MatMul";

  SizeType key_dim = 4;

  // create input
  TypeParam query_data = TypeParam({12, 25, 4});
  TypeParam key_data   = query_data;
  TypeParam value_data = query_data;
  TypeParam mask_data  = TypeParam({25, 25, 4});
  mask_data.Fill(static_cast<DataType>(1));
  // create labels
  TypeParam labels({12, 25, 4});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(key_dim, static_cast<DataType>(1));

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

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
  layer.SetInput("ScaledDotProductAttention_Query", query_data);
  layer.SetInput("ScaledDotProductAttention_Key", key_data);
  layer.SetInput("ScaledDotProductAttention_Value", value_data);
  layer.SetInput("ScaledDotProductAttention_Mask", mask_data);
  TypeParam prediction = layer.Evaluate(output_name, true);

  layer2.SetInput("ScaledDotProductAttention_Query", query_data);
  layer2.SetInput("ScaledDotProductAttention_Key", key_data);
  layer2.SetInput("ScaledDotProductAttention_Value", value_data);
  layer2.SetInput("ScaledDotProductAttention_Mask", mask_data);
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

  layer.SetInput("ScaledDotProductAttention_Query", query_data);
  layer.SetInput("ScaledDotProductAttention_Key", key_data);
  layer.SetInput("ScaledDotProductAttention_Value", value_data);
  layer.SetInput("ScaledDotProductAttention_Mask", mask_data);
  TypeParam prediction3 = layer.Evaluate(output_name);

  layer2.SetInput("ScaledDotProductAttention_Query", query_data);
  layer2.SetInput("ScaledDotProductAttention_Key", key_data);
  layer2.SetInput("ScaledDotProductAttention_Value", value_data);
  layer2.SetInput("ScaledDotProductAttention_Mask", mask_data);
  TypeParam prediction4 = layer2.Evaluate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}
