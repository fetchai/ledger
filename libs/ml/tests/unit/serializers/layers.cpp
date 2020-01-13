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
#include "math/base_types.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"

#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SaveParamsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SaveParamsTest, math::test::TensorFloatingTypes);

TYPED_TEST(SaveParamsTest, conv1d_saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;
  using LayerType  = fetch::ml::layers::Convolution1D<TensorType>;
  using SPType     = typename LayerType::SPType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  std::string input_name  = "Conv1D_Input";
  std::string output_name = "Conv1D_Conv1D";

  // Generate input
  TensorType input({input_channels, input_height, 1});
  input.FillUniformRandom();

  TensorType labels({output_channels, 1, 1});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(output_channels, input_channels, kernel_height, stride_size);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("label", {});

  // Add loss function
  std::string error_output =
      layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
          "num_error", {output_name, label_name});

  // set input and evaluate
  layer.SetInput(input_name, input);
  TensorType prediction;
  // make initial prediction to set internal buffers which must be correctly set in serialisation
  prediction = layer.Evaluate(output_name, true);

  // extract saveparams
  auto sp = layer.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b{};
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild
  auto layer2 = *(fetch::ml::utilities::BuildLayer<TensorType, LayerType>(dsp2));

  // test equality
  layer.SetInput(input_name, input);
  prediction = layer.Evaluate(output_name, true);

  layer2.SetInput(input_name, input);
  TensorType prediction2 = layer2.Evaluate(output_name, true);

  EXPECT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TensorType loss = layer.Evaluate(error_output);
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TensorType loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

  EXPECT_TRUE(loss.AllClose(loss2, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  // new random input
  input.FillUniformRandom();

  layer.SetInput(input_name, input);
  TensorType prediction3 = layer.Evaluate(output_name);

  layer2.SetInput(input_name, input);
  TensorType prediction4 = layer2.Evaluate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SaveParamsTest, conv2d_saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;
  using LayerType  = fetch::ml::layers::Convolution2D<TensorType>;
  using SPType     = typename LayerType::SPType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  std::string input_name  = "Conv2D_Input";
  std::string output_name = "Conv2D_Conv2D";

  // Generate input
  TensorType input({input_channels, input_height, input_width, 1});
  input.FillUniformRandom();

  TensorType labels({output_channels, 1, 1, 1});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(output_channels, input_channels, kernel_height, stride_size);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("label", {});

  // Add loss function
  std::string error_output =
      layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
          "num_error", {output_name, label_name});

  // set input and evaluate
  layer.SetInput(input_name, input);
  TensorType prediction;
  // make initial prediction to set internal buffers which must be correctly set in serialisation
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
  auto layer2 = *(fetch::ml::utilities::BuildLayer<TensorType, LayerType>(dsp2));

  // test equality
  layer.SetInput(input_name, input);
  prediction = layer.Evaluate(output_name, true);
  layer2.SetInput(input_name, input);
  TensorType prediction2 = layer2.Evaluate(output_name, true);

  ASSERT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TensorType loss = layer.Evaluate(error_output);
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TensorType loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

  EXPECT_TRUE(loss.AllClose(loss2, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  // new random input
  input.FillUniformRandom();

  layer.SetInput(input_name, input);
  TensorType prediction3 = layer.Evaluate(output_name);

  layer2.SetInput(input_name, input);
  TensorType prediction4 = layer2.Evaluate(output_name, true);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SaveParamsTest, fully_connected_saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = fetch::math::SizeType;
  using LayerType = fetch::ml::layers::FullyConnected<TypeParam>;
  using SPType    = typename LayerType::SPType;

  SizeType data_size       = 10;
  SizeType input_features  = 10;
  SizeType output_features = 20;

  std::string input_name  = "FullyConnected_Input";
  std::string output_name = "FullyConnected_Add";

  // create input
  TypeParam input({data_size, input_features});
  input.FillUniformRandom();

  // create labels
  TypeParam labels({output_features, data_size});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(input_features, output_features);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and evaluate
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

  EXPECT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TypeParam loss = layer.Evaluate(error_output);
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

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

TYPED_TEST(SaveParamsTest, layer_norm_saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using LayerType = fetch::ml::layers::LayerNorm<TypeParam>;
  using SPType    = typename LayerType::SPType;

  std::string input_name  = "LayerNorm_Input";
  std::string output_name = "LayerNorm_Beta_Addition";

  std::vector<fetch::math::SizeType> data_shape = {3, 2};
  TypeParam                          input      = TypeParam::FromString(
      "1, 2, 3, 0;"
      "2, 3, 2, 1;"
      "3, 6, 4, 13");
  input.Reshape({3, 2, 2});

  TypeParam labels({3, 2, 2});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(data_shape);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and evaluate
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
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

  EXPECT_TRUE(loss.AllClose(loss2, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  // new random input
  input.FillUniformRandom();

  layer.SetInput(input_name, input);
  TypeParam prediction3 = layer.Evaluate(output_name);

  layer2.SetInput(input_name, input);
  TypeParam prediction4 = layer2.Evaluate(output_name);

  EXPECT_FALSE(prediction == prediction3);

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SaveParamsTest, multi_head_attention_saveparams_test)
{
  using LayerType = typename fetch::ml::layers::MultiheadAttention<TypeParam>;
  using SPType    = typename LayerType::SPType;
  using DataType  = typename TypeParam::Type;

  fetch::math::SizeType n_heads   = 3;
  fetch::math::SizeType model_dim = 6;

  std::string output_name = "MultiheadAttention_Final_Transformation";

  // create input data
  TypeParam query_data = TypeParam({6, 12, 3});
  query_data.FillUniformRandom();

  TypeParam key_data   = query_data.Copy();
  TypeParam value_data = query_data.Copy();

  TypeParam mask_data = TypeParam({12, 12, 3});
  mask_data.Fill(DataType{1});

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
  layer.SetInput("MultiheadAttention_Mask", mask_data);

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

TYPED_TEST(SaveParamsTest, prelu_saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using LayerType = typename fetch::ml::layers::PRelu<TypeParam>;
  using SPType    = typename LayerType::SPType;

  std::string input_name  = "PRelu_Input";
  std::string output_name = "PRelu_PReluOp";

  math::SizeType input_dim_0 = 5;
  math::SizeType input_dim_1 = 10;
  math::SizeType input_dim_2 = 2;
  TypeParam      input({input_dim_0, input_dim_1, input_dim_2});
  input.FillUniformRandom();

  TypeParam labels({input_dim_0, input_dim_1, input_dim_2});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(input_dim_0 * input_dim_1, "PRelu");

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and evaluate
  layer.SetInput(input_name, input);

  TypeParam prediction = layer.Evaluate(output_name, true);

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
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

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

TYPED_TEST(SaveParamsTest, scaled_dot_product_attention_saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = fetch::math::SizeType;
  using LayerType = typename fetch::ml::layers::ScaledDotProductAttention<TypeParam>;
  using SPType    = typename LayerType::SPType;

  std::string output_name = "ScaledDotProductAttention_Value_Weight_MatMul";

  SizeType key_dim = 4;

  // create input
  TypeParam query_data = TypeParam({12, 25, 4});
  TypeParam key_data   = query_data;
  TypeParam value_data = query_data;
  TypeParam mask_data  = TypeParam({25, 25, 4});
  query_data.Fill(fetch::math::Type<DataType>("0.1"));
  key_data.Fill(fetch::math::Type<DataType>("0.1"));
  value_data.Fill(fetch::math::Type<DataType>("0.1"));
  mask_data.Fill(fetch::math::Type<DataType>("1"));

  // create labels
  TypeParam labels({12, 25, 4});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(key_dim, DataType{1});

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

  EXPECT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TypeParam loss = layer.Evaluate(error_output);
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

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

TYPED_TEST(SaveParamsTest, self_attention_saveparams_test)
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

TYPED_TEST(SaveParamsTest, skipgram_saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = fetch::math::SizeType;
  using LayerType = typename fetch::ml::layers::SkipGram<TypeParam>;
  using SPType    = typename LayerType::SPType;

  SizeType in_size    = 1;
  SizeType out_size   = 1;
  SizeType embed_size = 1;
  SizeType vocab_size = 10;
  SizeType batch_size = 1;

  std::string output_name = "SkipGram_Sigmoid";

  // create input

  TypeParam input({1, batch_size});
  TypeParam context({1, batch_size});
  TypeParam labels({1, batch_size});
  input(0, 0)   = DataType{0};
  context(0, 0) = static_cast<DataType>(5);
  labels(0, 0)  = DataType{0};

  // Create layer
  LayerType layer(in_size, out_size, embed_size, vocab_size);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and ForwardPropagate
  layer.SetInput("SkipGram_Input", input);
  layer.SetInput("SkipGram_Context", context);

  // make initial prediction to set internal buffers which must be correctly set in serialisation
  TypeParam prediction0 = layer.Evaluate(output_name, true);

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

  //
  // test that deserialized model gives the same forward prediction as the original layer
  //
  layer.SetInput("SkipGram_Input", input);
  layer.SetInput("SkipGram_Context", context);
  TypeParam prediction = layer.Evaluate(output_name, true);

  // sanity check - serialisation should not affect initial prediction
  ASSERT_TRUE(prediction0.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  layer2.SetInput("SkipGram_Input", input);
  layer2.SetInput("SkipGram_Context", context);
  TypeParam prediction2 = layer2.Evaluate(output_name, true);

  ASSERT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TypeParam loss = layer.Evaluate(error_output);
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

  EXPECT_TRUE(loss.AllClose(loss2, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  //
  // test that prediction is different after a back prop and step have been completed
  //

  layer.SetInput("SkipGram_Input", input);      // resets node cache
  layer.SetInput("SkipGram_Context", context);  // resets node cache
  TypeParam prediction3 = layer.Evaluate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  //
  // test that the deserialized model gives the same result as the original layer after training
  //

  layer2.SetInput("SkipGram_Input", input);
  layer2.SetInput("SkipGram_Context", context);
  TypeParam prediction5 = layer2.Evaluate(output_name);

  EXPECT_TRUE(prediction3.AllClose(prediction5, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
