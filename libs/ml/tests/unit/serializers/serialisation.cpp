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

#include "ml/core/graph.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"

// ordinary ops
#include "ml/ops/abs.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/avg_pool_1d.hpp"
#include "ml/ops/avg_pool_2d.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/constant.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/mask_fill.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/max_pool.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/maximum.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/one_hot.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/prelu_op.hpp"
#include "ml/ops/reduce_mean.hpp"
#include "ml/ops/slice.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/squeeze.hpp"
#include "ml/ops/switch.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/top_k.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"

// activations
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/elu.hpp"
#include "ml/ops/activations/gelu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"

// loss functions
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"

// metrics
#include "ml/ops/metrics/categorical_accuracy.hpp"

// layers
#include "gtest/gtest.h"
#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/multihead_attention.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/layers/skip_gram.hpp"

namespace {

using namespace fetch::ml;

template <typename T>
class SaveParamsTest : public ::testing::Test
{
};

template <typename T>
class SerializersTestWithInt : public ::testing::Test
{
};

template <typename T>
class SerializersTestNoInt : public ::testing::Test
{
};

template <typename T>
class GraphRebuildTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SaveParamsTest, math::test::TensorFloatingTypes);
TYPED_TEST_CASE(SerializersTestWithInt, math::test::TensorIntAndFloatingTypes);
TYPED_TEST_CASE(SerializersTestNoInt, math::test::TensorFloatingTypes);
TYPED_TEST_CASE(GraphRebuildTest, math::test::HighPrecisionTensorFloatingTypes);

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


///////////////////////
/// MATRIX MULTIPLY ///
///////////////////////

TYPED_TEST(SaveParamsTest, matrix_multiply_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MatrixMultiply<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MatrixMultiply<TensorType>;

  TypeParam data_1 = TypeParam::FromString("1, 2, -3, 4, 5");
  TypeParam data_2 = TypeParam::FromString(
      "-11, 12, 13, 14; 21, 22, 23, 24; 31, 32, 33, 34; 41, 42, 43, 44; 51, 52, 53, 54");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, matrix_multiply_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::MatrixMultiply<TensorType>;
  using SPType     = typename OpType::SPType;
  TypeParam a1({3, 4, 2});
  TypeParam b1({4, 3, 2});
  TypeParam error({3, 3, 2});
  TypeParam gradient_a({3, 4, 2});
  TypeParam gradient_b({4, 3, 2});

  fetch::ml::ops::MatrixMultiply<TypeParam> op;
  std::vector<TypeParam>                    backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(a1), std::make_shared<TypeParam>(b1)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(backpropagated_signals.at(1).AllClose(
      new_backpropagated_signals.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL ///
///////////////////

TYPED_TEST(SaveParamsTest, maxpool_saveparams_test_1d)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool<TensorType>;
  using SizeType      = typename TypeParam::SizeType;

  TensorType          data({2, 5, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 9, 9});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) = fetch::math::AsType<DataType>(data_input[i * 5 + j]) +
                          fetch::math::AsType<DataType>(i_b * 10);
      }
    }

    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 2; ++j)
      {
        gt(i, j, i_b) = fetch::math::AsType<DataType>(gt_input[i * 2 + j]) +
                        fetch::math::AsType<DataType>(i_b * 10);
      }
    }
  }

  OpType op(4, 1);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, maxpool_saveparams_backward_test_1d_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = fetch::math::AsType<DataType>(data_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = fetch::math::AsType<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool<TensorType> op(4, 1);
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(SaveParamsTest, maxpool_saveparams_test_2d)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool2D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SizeType      = typename TypeParam::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const batch_size = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  OpType op(3, 2);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, maxpool_saveparams_backward_2_channels_test_2d)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SPType     = typename OpType::SPType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});
  TensorType error({channels_size, output_width, output_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL 1D ///
///////////////////

TYPED_TEST(SaveParamsTest, maxpool_1d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool1D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool1D<TensorType>;
  using SizeType      = fetch::math::SizeType;

  TensorType          data({2, 5, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) = fetch::math::AsType<DataType>(data_input[i * 5 + j]) +
                          fetch::math::AsType<DataType>(i_b * 10);
      }
    }
  }

  OpType op(4, 1);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, maxpool_1d_saveparams_backward_test_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool1D<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = fetch::math::AsType<DataType>(data_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = fetch::math::AsType<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(4, 1);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////////
/// MAX POOL 2D ///
///////////////////

TYPED_TEST(SaveParamsTest, maxpool_2d_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::MaxPool2D<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SizeType      = fetch::math::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const batch_size = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  OpType op(3, 2);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, maxpool_2d_saveparams_backward_2_channels_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::MaxPool2D<TensorType>;
  using SPType     = typename OpType::SPType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  TensorType data({channels_size, input_width, input_height, batch_size});
  TensorType error({channels_size, output_width, output_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = fetch::math::AsType<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  fetch::ml::ops::MaxPool2D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

///////////////
/// MAXIMUM ///
///////////////

TYPED_TEST(SaveParamsTest, maximum_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Maximum<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Maximum<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, maximum_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Maximum<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Maximum<TensorType> op;
  std::vector<TensorType>             prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////
/// MULTIPLY ///
////////////////

TYPED_TEST(SaveParamsTest, multiply_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Multiply<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Multiply<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, multiply_saveparams_backward_test_NB_NB)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Multiply<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  ASSERT_TRUE(!fetch::math::state_overflow<typename TypeParam::Type>());
}

///////////////
/// ONE-HOT ///
///////////////

TYPED_TEST(SaveParamsTest, one_hot_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::OneHot<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::OneHot<TensorType>;

  TensorType data = TypeParam::FromString("1,0,1,2");
  data.Reshape({2, 2, 1, 1});

  SizeType depth     = 3;
  SizeType axis      = 3;
  auto     on_value  = DataType{5};
  auto     off_value = DataType{-1};

  OpType op(depth, axis, on_value, off_value);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

///////////////////
/// PLACEHOLDER ///
///////////////////

TYPED_TEST(SaveParamsTest, placeholder_saveable_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::PlaceHolder<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::PlaceHolder<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpType op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // placeholders do not store their data in serialisation, so we re set the data here
  new_op.SetData(data);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

////////////////
/// PRELU_OP ///
////////////////

TYPED_TEST(SaveParamsTest, prelu_op_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::PReluOp<TensorType>::SPType;
  using OpType        = fetch::ml::ops::PReluOp<TensorType>;

  TensorType data =
      TensorType::FromString("1, -2, 3,-4, 5,-6, 7,-8; -1,  2,-3, 4,-5, 6,-7, 8").Transpose();

  TensorType alpha = TensorType::FromString("0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8").Transpose();

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}));
  VecTensorType vec_data({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, prelu_op_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::PReluOp<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType alpha =
      TensorType::FromString(R"(0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8)").Transpose();

  TensorType data = TensorType::FromString(R"(1, -2, 3,-4, 5,-6, 7,-8;
                                             -1,  2,-3, 4,-5, 6,-7, 8)")
                        .Transpose();

  TensorType error = TensorType::FromString(R"(0, 0, 0, 0, 1, 1, 0, 0;
                                               0, 0, 0, 0, 1, 1, 0, 0)")
                         .Transpose();

  fetch::ml::ops::PReluOp<TensorType> op;
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction =
      op.Backward({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

/////////////////////////
/// REDUCE MEAN TESTS ///
/////////////////////////

TYPED_TEST(SaveParamsTest, reduce_mean_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::ReduceMean<TensorType>::SPType;
  using OpType        = fetch::ml::ops::ReduceMean<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  OpType op(1);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SaveParamsTest, reduce_mean_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::ReduceMean<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({2, 1, 2});

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SaveParamsTest, ReduceMean_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::ReduceMean<TensorType>>("Output", {input_name}, 1);

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
  fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

/////////////////////
/// RESHAPE TESTS ///
/////////////////////

TYPED_TEST(SaveParamsTest, Reshape_graph_serialisation_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  std::vector<math::SizeType> final_shape({8, 1, 1, 1});

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::Reshape<TensorType>>("Output", {input_name}, final_shape);

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
  fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SaveParamsTest, reshape_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Reshape<TensorType>::SPType;

  // construct tensor & reshape op
  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});
  fetch::ml::ops::Reshape<TensorType> op({8, 1, 1, 1});

  // forward pass
  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});
  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  fetch::ml::ops::Reshape<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, reshape_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Reshape<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2, 1});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({8, 1, 1});

  // call reshape and store result in 'error'
  fetch::ml::ops::Reshape<TypeParam>             op({8, 1, 1});
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

/////////////
/// SLICE ///
/////////////

TYPED_TEST(SaveParamsTest, slice_single_axis_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeVector    = typename TypeParam::SizeVector;

  TypeParam  data({1, 2, 3, 4, 5});
  SizeVector axes({3});
  SizeVector indices({3});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

  // forward pass
  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});
  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, slice_single_axis_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Slice<TensorType>;
  using SPType     = typename OpType::SPType;
  using SizeType   = fetch::math::SizeType;

  TypeParam data = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  data.Reshape({3, 2, 2});
  SizeType axis(1u);
  SizeType index(0u);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3");
  error.Reshape({3, 1, 2});

  fetch::ml::ops::Slice<TypeParam>               op(index, axis);
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SaveParamsTest, slice_ranged_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeType      = fetch::math::SizeType;
  using SizePairType  = std::pair<SizeType, SizeType>;

  TypeParam data = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  data.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  fetch::ml::ops::Slice<TypeParam> op(start_end_slice, axis);

  // forward pass
  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});
  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, slice_ranged_saveparams_backward_test)
{
  using TensorType   = TypeParam;
  using DataType     = typename TypeParam::Type;
  using OpType       = fetch::ml::ops::Slice<TensorType>;
  using SPType       = typename OpType::SPType;
  using SizeType     = fetch::math::SizeType;
  using SizePairType = std::pair<SizeType, SizeType>;

  TypeParam data = TypeParam::FromString("1, 1, 3, 141; 4, 52, 6, 72; -1, -2, -19, -4");
  data.Reshape({3, 2, 2});

  SizeType     axis{0};
  SizePairType start_end_slice = std::make_pair(1, 3);

  TypeParam error = TypeParam::FromString("1, 3; 4, 6; -1, -3; -2, -3");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::Slice<TypeParam>               op(start_end_slice, axis);
  std::vector<std::shared_ptr<const TensorType>> data_vec({std::make_shared<TensorType>(data)});
  op.Forward(data_vec, error);

  // backprop with incoming error signal matching output data
  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

///////////////////
/// SLICE TESTS ///
///////////////////

TYPED_TEST(SaveParamsTest, slice_multi_axes_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Slice<TensorType>::SPType;
  using SizeVector    = typename TypeParam::SizeVector;

  TypeParam data = TypeParam::FromString("1, 2, 3, 4; 4, 5, 6, 7; -1, -2, -3, -4");
  data.Reshape({3, 2, 2});
  SizeVector axes({1, 2});
  SizeVector indices({1, 1});

  fetch::ml::ops::Slice<TypeParam> op(indices, axes);

  // forward pass
  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});
  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  fetch::ml::ops::Slice<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

////////////
/// SQRT ///
////////////

TYPED_TEST(SaveParamsTest, sqrt_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Sqrt<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Sqrt<TensorType>;

  TensorType data = TensorType::FromString("0, 1, 2, 4, 10, 100");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, sqrt_saveparams_backward_all_positive_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Sqrt<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data  = TensorType::FromString("1,   2,         4,   10,       100");
  TensorType error = TensorType::FromString("1,   1,         1,    2,         0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

/////////////////////
/// SQUEEZE TESTS ///
/////////////////////

TYPED_TEST(SaveParamsTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Squeeze<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Squeeze<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  OpType op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SaveParamsTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Squeeze<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({5});

  fetch::ml::ops::Squeeze<TypeParam> op;

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SaveParamsTest, squeeze_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::Squeeze<TensorType>>("Output", {input_name});

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
  fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

/////////////////////
/// STRIDED SLICE ///
/////////////////////

TYPED_TEST(SaveParamsTest, strided_slice_saveparams_test)
{
  using TensorType    = TypeParam;
  using SizeType      = fetch::math::SizeType;
  using SizeVector    = typename TypeParam::SizeVector;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::StridedSlice<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::StridedSlice<TensorType>;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam gt({6, 4, 3, 1, 2});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = input.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  for (SizeType i{0}; i < gt.shape().at(0); i++)
  {
    for (SizeType j{0}; j < gt.shape().at(1); j++)
    {
      for (SizeType k{0}; k < gt.shape().at(2); k++)
      {
        for (SizeType l{0}; l < gt.shape().at(3); l++)
        {
          for (SizeType m{0}; m < gt.shape().at(4); m++)
          {
            gt.At(i, j, k, l, m) =
                input.At(begins.at(0) + i * strides.at(0), begins.at(1) + j * strides.at(1),
                         begins.at(2) + k * strides.at(2), begins.at(3) + l * strides.at(3),
                         begins.at(4) + m * strides.at(4));
          }
        }
      }
    }
  }

  OpType op(begins, ends, strides);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(input)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(input)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, strided_slice_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::StridedSlice<TensorType>;
  using SPType     = typename OpType::SPType;
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TypeParam::SizeVector;
  using DataType   = typename TypeParam::Type;

  TypeParam input({9, 9, 9, 6, 4});
  TypeParam error({6, 4, 3, 1, 2});

  SizeVector begins({3, 1, 0, 4, 0});
  SizeVector ends({8, 7, 8, 5, 2});
  SizeVector strides({1, 2, 3, 4, 2});

  auto     it  = error.begin();
  SizeType cnt = 0;
  while (it.is_valid())
  {
    *it = static_cast<DataType>(cnt);
    cnt++;
    ++it;
  }

  fetch::ml::ops::StridedSlice<TypeParam> op(begins, ends, strides);
  std::vector<TypeParam>                  backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(input)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals = op.Backward({std::make_shared<TypeParam>(input)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(input)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////////
/// SUBTRACT ///
////////////////

TYPED_TEST(SaveParamsTest, subtract_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Subtract<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Subtract<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      " 8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, subtract_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::Subtract<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Subtract<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

//////////////
/// SWITCH ///
//////////////

TYPED_TEST(SaveParamsTest, switch_saveparams_back_test_broadcast_mask)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = typename fetch::ml::ops::Switch<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType mask = TensorType::FromString("1, 1, 0");
  mask.Reshape({1, 3, 1});

  TensorType target_input = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  target_input.Reshape({3, 3, 1});

  TensorType mask_value({3, 3, 1});
  mask_value.Fill(static_cast<DataType>(-100));

  TensorType error_signal = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9");
  error_signal.Reshape({3, 3, 1});

  fetch::ml::ops::Switch<TensorType> op;

  std::vector<TypeParam> prediction = op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_prediction = new_op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(2).AllClose(
      new_prediction.at(2), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////
/// TANH ///
////////////

TYPED_TEST(SaveParamsTest, tanh_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TanH<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::TanH<TensorType>;

  TensorType data = TypeParam::FromString("0, 0.2, 0.4, -0, -0.2, -0.4");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, tanh_saveparams_backward_all_negative_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::TanH<TensorType>;
  using SPType     = typename OpType::SPType;

  uint8_t    n = 8;
  TypeParam  data{n};
  TypeParam  error{n};
  TensorType data_input  = TensorType::FromString("-0, -0.2, -0.4, -0.6, -0.8, -1.2, -1.4, -10");
  TensorType error_input = TensorType::FromString("-0.2, -0.1, -0.3, -0.2, -0.5, -0.1, -0.0, -0.3");

  for (uint64_t i(0); i < n; ++i)
  {
    data.Set(i, data_input[i]);
    error.Set(i, error_input[i]);
  }

  fetch::ml::ops::TanH<TypeParam> op;
  std::vector<TypeParam> prediction = op.Backward({std::make_shared<const TypeParam>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

////////////
/// TOPK ///
////////////

TYPED_TEST(SaveParamsTest, top_k_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using SizeType      = fetch::math::SizeType;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TopK<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::TopK<TensorType>;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});

  SizeType k      = 2;
  bool     sorted = true;

  OpType op(k, sorted);

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, top_k_saveparams_backward_test)
{
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::TopK<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TypeParam::FromString("9,4,3,2;5,6,7,8;1,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  TensorType error = TypeParam::FromString("20,-21,22,-23;24,-25,26,-27");
  error.Reshape({2, 4});

  SizeType k      = 2;
  bool     sorted = true;

  fetch::ml::ops::TopK<TypeParam> op(k, sorted);

  // Run forward pass before backward pass
  TypeParam prediction(op.ComputeOutputShape({std::make_shared<TypeParam>(data)}));
  op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // Run forward pass before backward pass
  new_op.Forward({std::make_shared<TypeParam>(data)}, prediction);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

/////////////////
/// TRANSPOSE ///
/////////////////

TYPED_TEST(SaveParamsTest, transpose_saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Transpose<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Transpose<TensorType>;

  TensorType data = TypeParam::FromString(R"(1, 2, -3; 4, 5, 6)");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, transpose_saveparams_backward_batch_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Transpose<TensorType>;
  using SPType     = typename OpType::SPType;
  TypeParam a({4, 5, 2});
  TypeParam error({5, 4, 2});

  fetch::ml::ops::Transpose<TypeParam> op;
  std::vector<TypeParam>               backpropagated_signals =
      op.Backward({std::make_shared<TypeParam>(a)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  backpropagated_signals = op.Backward({std::make_shared<TypeParam>(a)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_backpropagated_signals =
      new_op.Backward({std::make_shared<TypeParam>(a)}, error);

  // test correct values
  EXPECT_TRUE(backpropagated_signals.at(0).AllClose(
      new_backpropagated_signals.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

TYPED_TEST(SaveParamsTest, weights_saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::Weights<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::Weights<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpType op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(SaveParamsTest, weights_saveparams_gradient_step_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::Weights<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType       data(8);
  TensorType       error(8);
  std::vector<int> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  for (SizeType i{0}; i < 8; ++i)
  {
    data.Set(i, static_cast<DataType>(dataInput[i]));
    error.Set(i, static_cast<DataType>(errorInput[i]));
  }

  fetch::ml::ops::Weights<TensorType> op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  std::vector<TensorType> error_signal = op.Backward({}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  op.Backward({}, error);

  TensorType grad = op.GetGradientsReferences();
  fetch::math::Multiply(grad, DataType{-1}, grad);
  op.ApplyGradient(grad);

  prediction = TensorType(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  new_op.Backward({}, error);

  TensorType new_grad = new_op.GetGradientsReferences();
  fetch::math::Multiply(new_grad, DataType{-1}, new_grad);
  new_op.ApplyGradient(new_grad);

  TensorType new_prediction = TensorType(new_op.ComputeOutputShape({}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(new_prediction,
                                  fetch::math::function_tolerance<typename TypeParam::Type>(),
                                  fetch::math::function_tolerance<typename TypeParam::Type>()));
}



//////////////////////////////
/// GRAPH OP SERIALISATION ///
//////////////////////////////



template <typename OpType, typename GraphPtrType, typename... Params>
std::string AddOp(GraphPtrType g, std::vector<std::string> input_nodes, Params... params)
{
  return g->template AddNode<OpType>("", input_nodes, params...);
}

template <typename GraphPtrType, typename TensorType>
void ComparePrediction(GraphPtrType g, GraphPtrType g2, std::string node_name)
{
  using DataType         = typename TensorType::Type;
  TensorType prediction  = g->Evaluate(node_name);
  TensorType prediction2 = g2->Evaluate(node_name);
  EXPECT_TRUE(prediction.AllClose(prediction2, DataType{0}, DataType{0}));
}

TYPED_TEST(SerializersTestWithInt, serialize_empty_state_dict)
{
  fetch::ml::StateDict<TypeParam>       sd1;
  fetch::serializers::MsgPackSerializer b;
  b << sd1;
  b.seek(0);
  fetch::ml::StateDict<TypeParam> sd2;
  b >> sd2;
  EXPECT_EQ(sd1, sd2);
}

TYPED_TEST(SerializersTestNoInt, serialize_state_dict)
{
  // Generate a plausible state dict out of a fully connected layer
  fetch::ml::layers::FullyConnected<TypeParam> fc(10, 10);
  struct fetch::ml::StateDict<TypeParam>       sd1 = fc.StateDict();
  fetch::serializers::MsgPackSerializer        b;
  b << sd1;
  b.seek(0);
  fetch::ml::StateDict<TypeParam> sd2;
  b >> sd2;
  EXPECT_EQ(sd1, sd2);
}

TYPED_TEST(SerializersTestWithInt, serialize_empty_graph_saveable_params)
{
  fetch::ml::GraphSaveableParams<TypeParam> gsp1;
  fetch::serializers::MsgPackSerializer     b;
  b << gsp1;
  b.seek(0);
  fetch::ml::GraphSaveableParams<TypeParam> gsp2;
  b >> gsp2;
  EXPECT_EQ(gsp1.connections, gsp2.connections);
  EXPECT_EQ(gsp1.nodes, gsp2.nodes);
}

TYPED_TEST(SerializersTestNoInt, serialize_graph_saveable_params)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using GraphType  = typename fetch::ml::Graph<TensorType>;

  fetch::ml::RegularisationType regulariser = fetch::ml::RegularisationType::L1;
  auto                          reg_rate    = fetch::math::Type<DataType>("0.01");

  // Prepare graph with fairly random architecture
  auto g = std::make_shared<GraphType>();

  std::string input = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label_name =
      g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("label", {});

  std::string layer_1 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {input}, 10u, 20u, fetch::ml::details::ActivationType::RELU, regulariser, reg_rate);
  std::string layer_2 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC2", {layer_1}, 20u, 10u, fetch::ml::details::ActivationType::RELU, regulariser, reg_rate);
  std::string output = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC3", {layer_2}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX, regulariser,
      reg_rate);

  // Add loss function
  std::string error_output = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
      "num_error", {output, label_name});

  /// make a prediction and do nothing with it
  TensorType tmp_data = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
  g->SetInput("Input", tmp_data.Transpose());
  TensorType tmp_prediction = g->Evaluate(output);

  fetch::ml::GraphSaveableParams<TypeParam>      gsp1 = g->GetGraphSaveableParams();
  fetch::serializers::LargeObjectSerializeHelper b;
  b.Serialize(gsp1);

  auto gsp2 = std::make_shared<fetch::ml::GraphSaveableParams<TypeParam>>();

  b.Deserialize(*gsp2);
  EXPECT_EQ(gsp1.connections, gsp2->connections);

  for (auto const &gsp2_node_pair : gsp2->nodes)
  {
    auto gsp2_node = gsp2_node_pair.second;
    auto gsp1_node = gsp1.nodes[gsp2_node_pair.first];

    EXPECT_TRUE(gsp1_node->operation_type == gsp2_node->operation_type);
  }

  auto g2 = std::make_shared<GraphType>();
  fetch::ml::utilities::BuildGraph<TensorType>(*gsp2, g2);

  TensorType data   = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
  TensorType labels = TensorType::FromString("1; 2; 3; 4; 5; 6; 7; 8; 9; 100");

  g->SetInput("Input", data.Transpose());
  g2->SetInput("Input", data.Transpose());

  TensorType prediction  = g->Evaluate(output);
  TensorType prediction2 = g2->Evaluate(output);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  g->SetInput(label_name, labels);
  g->Evaluate(error_output);
  g->BackPropagate(error_output);
  auto grads = g->GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  g->ApplyGradients(grads);

  // train g2
  g2->SetInput(label_name, labels);
  g2->Evaluate(error_output);
  g2->BackPropagate(error_output);
  auto grads2 = g2->GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  g2->ApplyGradients(grads2);

  g->SetInput("Input", data.Transpose());
  TensorType prediction3 = g->Evaluate(output);

  g2->SetInput("Input", data.Transpose());
  TensorType prediction4 = g2->Evaluate(output);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphRebuildTest, graph_rebuild_every_op)
{
  using TensorType   = TypeParam;
  using DataType     = typename TensorType::Type;
  using GraphType    = fetch::ml::Graph<TypeParam>;
  using GraphPtrType = std::shared_ptr<GraphType>;

  // setup input data
  TensorType data1       = TensorType::FromString(R"(1 , 1 , 1, 2 , 3 , 4)");
  TensorType data2       = TensorType::FromString(R"(-20,-10, 1, 10, 20, 30)");
  TensorType data_3d     = TensorType::FromString(R"(1, 1, 1, 2 , 3 , 2, 1, 2)");
  TensorType data_4d     = TensorType::FromString(R"(-1, 1, 1, 2 , 3 , 2, 1, 2)");
  TensorType data_5d     = TensorType::FromString(R"(-1, 1, 1, 2 , 3 , 2, 1, 2)");
  TensorType data_binary = TensorType::FromString(R"(1 , 1 , 0, 0 , 0 , 1)");
  TensorType data_logits = TensorType::FromString(R"(0.2 , 0.2 , 0.2, 0.2 , 0.1 , 0.1)");
  TensorType data_embed({5, 5});
  TensorType query_data = TensorType({12, 25, 4});
  query_data.Fill(DataType{0});
  TensorType key_data   = query_data;
  TensorType value_data = query_data;
  TensorType mask_data  = TensorType({25, 25, 4});
  data_3d.Reshape({2, 2, 2});
  data_4d.Reshape({2, 2, 2, 1});
  data_5d.Reshape({2, 2, 2, 1, 1});
  TensorType data_1_2_4 = data1.Copy();
  data_1_2_4.Reshape({2, 4});

  // Create graph
  std::string name = "Graph";
  auto        g    = std::make_shared<GraphType>();

  // placeholder inputs
  std::string input_1                = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_1_transpose      = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_1_2_4            = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_2                = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_3d               = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_4d               = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_5d               = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_binary           = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_binary_transpose = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_logits           = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_logits_transpose = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_query            = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_key              = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_value            = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_mask             = AddOp<ops::PlaceHolder<TensorType>>(g, {});

  // ordinary ops
  std::string abs          = AddOp<ops::Abs<TensorType>>(g, {input_1});
  std::string add          = AddOp<ops::Add<TensorType>>(g, {input_1, input_2});
  std::string avg1         = AddOp<ops::AvgPool1D<TensorType>>(g, {input_3d}, 1, 1);
  std::string avg2         = AddOp<ops::AvgPool2D<TensorType>>(g, {input_4d}, 1, 1);
  std::string concat       = AddOp<ops::Concatenate<TensorType>>(g, {input_1, input_2}, 0);
  std::string constant     = AddOp<ops::Constant<TensorType>>(g, {});
  std::string conv1d       = AddOp<ops::Convolution1D<TensorType>>(g, {input_3d, input_4d});
  std::string conv2d       = AddOp<ops::Convolution2D<TensorType>>(g, {input_4d, input_5d});
  std::string divide       = AddOp<ops::Divide<TensorType>>(g, {input_1, input_2});
  std::string embed        = AddOp<ops::Embeddings<TensorType>>(g, {input_1}, data_embed);
  std::string exp          = AddOp<ops::Exp<TensorType>>(g, {input_1});
  std::string flatten      = AddOp<ops::Flatten<TensorType>>(g, {input_1});
  std::string layernorm_op = AddOp<ops::LayerNorm<TensorType>>(g, {input_1});
  std::string log          = AddOp<ops::Log<TensorType>>(g, {input_1});
  std::string maskfill     = AddOp<ops::MaskFill<TensorType>>(g, {input_1, input_1}, DataType{0});
  std::string matmul      = AddOp<ops::MatrixMultiply<TensorType>>(g, {input_1, input_1_transpose});
  std::string maxpool     = AddOp<ops::MaxPool<TensorType>>(g, {input_3d}, 1, 1);
  std::string maxpool1d   = AddOp<ops::MaxPool1D<TensorType>>(g, {input_3d}, 1, 1);
  std::string maxpool2d   = AddOp<ops::MaxPool2D<TensorType>>(g, {input_4d}, 1, 1);
  std::string maximum     = AddOp<ops::Maximum<TensorType>>(g, {input_1, input_2});
  std::string multiply    = AddOp<ops::Multiply<TensorType>>(g, {input_1, input_2});
  std::string onehot      = AddOp<ops::OneHot<TensorType>>(g, {input_1}, data1.size());
  std::string placeholder = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string prelu       = AddOp<ops::PReluOp<TensorType>>(g, {input_1, input_1_transpose});
  std::string reducemean  = AddOp<ops::ReduceMean<TensorType>>(g, {input_1}, 0);
  std::string slice       = AddOp<ops::Slice<TensorType>>(g, {input_1}, 0, 0);
  std::string sqrt        = AddOp<ops::Sqrt<TensorType>>(g, {input_1});
  std::string squeeze     = AddOp<ops::Squeeze<TensorType>>(g, {input_1});
  std::string switchop    = AddOp<ops::Switch<TensorType>>(g, {input_1, input_1, input_1});
  std::string tanh        = AddOp<ops::TanH<TensorType>>(g, {input_1});
  std::string transpose   = AddOp<ops::Transpose<TensorType>>(g, {input_1});
  std::string topk        = AddOp<ops::TopK<TensorType>>(g, {input_1_2_4}, 2);
  std::string weights     = AddOp<ops::Weights<TensorType>>(g, {});

  // activations
  std::string dropout =
      AddOp<ops::Dropout<TensorType>>(g, {input_1}, fetch::math::Type<DataType>("0.9"));
  std::string elu  = AddOp<ops::Elu<TensorType>>(g, {input_1}, fetch::math::Type<DataType>("0.9"));
  std::string gelu = AddOp<ops::Gelu<TensorType>>(g, {input_1});
  std::string leakyrelu  = AddOp<ops::LeakyRelu<TensorType>>(g, {input_1});
  std::string logsigmoid = AddOp<ops::LogSigmoid<TensorType>>(g, {input_1});
  std::string logsoftmax = AddOp<ops::LogSoftmax<TensorType>>(g, {input_1});
  std::string randomisedrelu =
      AddOp<ops::RandomisedRelu<TensorType>>(g, {input_1}, DataType{0}, DataType{1});
  std::string relu    = AddOp<ops::Relu<TensorType>>(g, {input_1});
  std::string sigmoid = AddOp<ops::Sigmoid<TensorType>>(g, {input_1});
  std::string softmax = AddOp<ops::Softmax<TensorType>>(g, {input_1});

  // Loss functions
  std::string cel  = AddOp<ops::CrossEntropyLoss<TensorType>>(g, {input_logits, input_binary});
  std::string mse  = AddOp<ops::MeanSquareErrorLoss<TensorType>>(g, {input_1, input_2});
  std::string scel = AddOp<ops::SoftmaxCrossEntropyLoss<TensorType>>(
      g, {input_logits_transpose, input_binary_transpose});

  // Metrics
  std::string acc = AddOp<ops::CategoricalAccuracy<TensorType>>(
      g, {input_logits_transpose, input_binary_transpose});

  // Layers
  std::string layer_layernorm =
      AddOp<layers::LayerNorm<TensorType>>(g, {input_1}, std::vector<math::SizeType>({1}));
  std::string layer_conv1d = AddOp<layers::Convolution1D<TensorType>>(g, {input_3d}, 1, 2, 1, 1);
  std::string layer_conv2d = AddOp<layers::Convolution2D<TensorType>>(g, {input_4d}, 1, 2, 1, 1);
  std::string layer_fc1    = AddOp<layers::FullyConnected<TensorType>>(g, {input_1}, 1, 1);
  std::string layer_mh     = AddOp<layers::MultiheadAttention<TensorType>>(
      g, {input_query, input_key, input_value, input_mask}, 4, 12);
  std::string layer_prelu = AddOp<layers::PRelu<TensorType>>(g, {input_1}, 1);
  std::string layer_scaleddotproductattention =
      AddOp<layers::ScaledDotProductAttention<TensorType>>(
          g, {input_query, input_key, input_value, input_mask}, 4);
  std::string layer_selfattentionencoder =
      AddOp<layers::SelfAttentionEncoder<TensorType>>(g, {input_query, input_mask}, 4, 12, 24);
  std::string layer_skipgram =
      AddOp<layers::SkipGram<TensorType>>(g, {input_1, input_1}, 1, 1, 10, 10);

  // assign input data
  g->SetInput(input_1, data1);
  g->SetInput(input_1_transpose, data1.Copy().Transpose());
  g->SetInput(input_1_2_4, data_1_2_4);
  g->SetInput(input_2, data2);
  g->SetInput(input_3d, data_3d);
  g->SetInput(input_4d, data_4d);
  g->SetInput(input_5d, data_5d);
  g->SetInput(constant, data1);
  g->SetInput(placeholder, data1);
  g->SetInput(weights, data1);
  g->SetInput(input_binary, data_binary);
  g->SetInput(input_binary_transpose, data_binary.Copy().Transpose());
  g->SetInput(input_logits, data_logits);
  g->SetInput(input_logits_transpose, data_logits.Copy().Transpose());
  g->SetInput(input_query, query_data);
  g->SetInput(input_key, key_data);
  g->SetInput(input_value, value_data);
  g->SetInput(input_mask, mask_data);
  g->Compile();

  // serialise the graph
  fetch::ml::GraphSaveableParams<TypeParam>      gsp1 = g->GetGraphSaveableParams();
  fetch::serializers::LargeObjectSerializeHelper b;
  b.Serialize(gsp1);

  // deserialise to a new graph
  auto gsp2 = std::make_shared<fetch::ml::GraphSaveableParams<TypeParam>>();
  b.Deserialize(*gsp2);
  EXPECT_EQ(gsp1.connections, gsp2->connections);

  for (auto const &gsp2_node_pair : gsp2->nodes)
  {
    auto gsp2_node = gsp2_node_pair.second;
    auto gsp1_node = gsp1.nodes[gsp2_node_pair.first];

    EXPECT_TRUE(gsp1_node->operation_type == gsp2_node->operation_type);
  }

  auto g2 = std::make_shared<GraphType>();
  fetch::ml::utilities::BuildGraph<TensorType>(*gsp2, g2);

  // evaluate both graphs to ensure outputs are identical
  g2->SetInput(input_1, data1);
  g2->SetInput(input_1_transpose, data1.Copy().Transpose());
  g2->SetInput(input_1_2_4, data_1_2_4);
  g2->SetInput(input_2, data2);
  g2->SetInput(input_3d, data_3d);
  g2->SetInput(input_4d, data_4d);
  g2->SetInput(input_5d, data_5d);
  g2->SetInput(constant, data1);
  g2->SetInput(placeholder, data1);
  g2->SetInput(weights, data1);
  g2->SetInput(input_binary, data_binary);
  g2->SetInput(input_binary_transpose, data_binary.Copy().Transpose());
  g2->SetInput(input_logits, data_logits);
  g2->SetInput(input_logits_transpose, data_logits.Copy().Transpose());
  g2->SetInput(input_query, query_data);
  g2->SetInput(input_key, key_data);
  g2->SetInput(input_value, value_data);
  g2->SetInput(input_mask, mask_data);
  g2->Compile();

  // weak tests that all ops produce the same value on both graphs
  // more thorough tests should be implemented in each test op file

  // ordinary ops
  ComparePrediction<GraphPtrType, TensorType>(g, g2, input_1);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, input_2);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, abs);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, add);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, avg1);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, avg2);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, concat);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, constant);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, conv1d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, conv2d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, divide);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, embed);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, exp);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, flatten);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layernorm_op);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, log);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maskfill);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, matmul);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maxpool);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maxpool1d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maxpool2d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maximum);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, multiply);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, onehot);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, placeholder);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, prelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, reducemean);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, slice);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, sqrt);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, squeeze);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, switchop);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, tanh);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, transpose);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, topk);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, weights);

  // activations
  ComparePrediction<GraphPtrType, TensorType>(g, g2, dropout);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, elu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, gelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, leakyrelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, logsigmoid);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, logsoftmax);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, randomisedrelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, relu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, sigmoid);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, softmax);

  // Loss functions
  ComparePrediction<GraphPtrType, TensorType>(g, g2, cel);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, mse);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, scel);

  // Metrics
  ComparePrediction<GraphPtrType, TensorType>(g, g2, acc);

  // Layers
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_layernorm);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_conv1d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_conv2d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_fc1);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_mh);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_prelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_scaleddotproductattention);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_selfattentionencoder);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_skipgram);
}

}  // namespace
