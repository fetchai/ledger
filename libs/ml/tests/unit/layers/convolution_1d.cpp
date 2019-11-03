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
#include "math/tensor.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>

template <typename T>
class Convolution1DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(Convolution1DTest, MyTypes);

TYPED_TEST(Convolution1DTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(std::vector<typename TypeParam::SizeType>({input_channels, input_height, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input(i_ic, i_i, 0) = static_cast<DataType>(i_i + 1);
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution1D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);
  conv.SetInput("Conv1D_Input", input);
  TypeParam output = conv.Evaluate("Conv1D_Conv1D", true);

  // test correct values
  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 1);
  ASSERT_EQ(output.shape()[2], 1);

  TensorType gt({output_channels, output_height});
  gt(0, 0) = static_cast<DataType>(-4.28031352977);
  gt(1, 0) = static_cast<DataType>(-4.03654768132);
  gt(2, 0) = static_cast<DataType>(8.11192789580);
  gt(3, 0) = static_cast<DataType>(1.763717529829592);
  gt(4, 0) = static_cast<DataType>(-1.8677866039798);

  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, ops_forward_test)  // Use the class as an Ops
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input(i_ic, i_i, 0) = static_cast<DataType>(i_i + 1);
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution1D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);

  TensorType output(conv.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  conv.Forward({std::make_shared<TypeParam>(input)}, output);

  // test correct values
  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 1);
  ASSERT_EQ(output.shape()[2], 1);

  TensorType gt({output_channels, output_height, 1});
  gt(0, 0, 0) = static_cast<DataType>(-4.28031352977);
  gt(1, 0, 0) = static_cast<DataType>(-4.03654768132);
  gt(2, 0, 0) = static_cast<DataType>(8.11192789580);
  gt(3, 0, 0) = static_cast<DataType>(1.763717529829592);
  gt(4, 0, 0) = static_cast<DataType>(-1.8677866039798);

  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, ops_backward_test)  // Use the class as an Ops
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input(i_ic, i_i, 0) = static_cast<DataType>(i_i + 1);
    }
  }

  // Generate error
  TensorType error_signal(
      std::vector<typename TypeParam::SizeType>({output_channels, output_height, 1}));

  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      error_signal(i_oc, i_o, 0) = static_cast<DataType>(2);
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution1D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);

  TensorType output(conv.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  conv.Forward({std::make_shared<TypeParam>(input)}, output);

  std::vector<TypeParam> backprop_error =
      conv.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  // test correct values
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], input_channels);
  ASSERT_EQ(backprop_error[0].shape()[1], input_height);
  ASSERT_EQ(backprop_error[0].shape()[2], 1);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 0, 0)), -4.3077492713928222656);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 0, 0)), 9.162715911865234375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 0, 0)), 0.80360949039459228516);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 1, 0)), 1.2491617202758789062);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 1, 0)), 2.8053097724914550781);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 1, 0)), -4.166011810302734375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 2, 0)), 2.4086174964904785156);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 2, 0)), -0.86411559581756591797);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 2, 0)), -3.5623354911804199219);
}

TYPED_TEST(Convolution1DTest, node_forward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(std::vector<typename TypeParam::SizeType>({input_channels, input_height, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input(i_ic, i_i, 0) = static_cast<DataType>(i_i + 1);
    }
  }

  // Evaluate
  auto placeholder_node = std::make_shared<fetch::ml::Node<TypeParam>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder_node->GetOp())
      ->SetData(input);

  auto conv = fetch::ml::Node<TypeParam>(
      fetch::ml::OpType::LAYER_CONVOLUTION_1D, "Convolution2D",
      [output_channels, input_channels, kernel_height, stride_size]() {
        return std::make_shared<fetch::ml::layers::Convolution1D<TypeParam>>(
            output_channels, input_channels, kernel_height, stride_size);
      });
  conv.AddInput(placeholder_node);

  TypeParam prediction = *conv.Evaluate(true);

  // test correct values
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 1);
  ASSERT_EQ(prediction.shape()[2], 1);

  TensorType gt({output_channels, output_height, 1});
  gt(0, 0, 0) = static_cast<DataType>(-4.28031352977);
  gt(1, 0, 0) = static_cast<DataType>(-4.03654768132);
  gt(2, 0, 0) = static_cast<DataType>(8.11192789580);
  gt(3, 0, 0) = static_cast<DataType>(1.763717529829592);
  gt(4, 0, 0) = static_cast<DataType>(-1.8677866039798);

  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, node_backward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input(i_ic, i_i, 0) = static_cast<DataType>(i_i + 1);
    }
  }

  // Generate error
  TensorType error_signal(
      std::vector<typename TypeParam::SizeType>({output_channels, output_height, 1}));

  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      error_signal(i_oc, i_o, 0) = static_cast<DataType>(2);
    }
  }

  // Evaluate
  auto placeholder_node = std::make_shared<fetch::ml::Node<TypeParam>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder_node->GetOp())
      ->SetData(input);

  auto conv = fetch::ml::Node<TypeParam>(
      fetch::ml::OpType::LAYER_CONVOLUTION_1D, "Convolution2D",
      [output_channels, input_channels, kernel_height, stride_size]() {
        return std::make_shared<fetch::ml::layers::Convolution1D<TypeParam>>(
            output_channels, input_channels, kernel_height, stride_size);
      });
  conv.AddInput(placeholder_node);

  TypeParam prediction     = *conv.Evaluate(true);
  auto      backprop_error = conv.BackPropagate(error_signal);

  // test correct values
  ASSERT_EQ(backprop_error.size(), 1);
  auto err_signal = (*(backprop_error.begin())).second.at(0);
  ASSERT_EQ(err_signal.shape().size(), 3);
  ASSERT_EQ(err_signal.shape()[0], input_channels);
  ASSERT_EQ(err_signal.shape()[1], input_height);
  ASSERT_EQ(err_signal.shape()[2], 1);

  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(0, 0, 0)), -4.3077492713928222656);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(1, 0, 0)), 9.162715911865234375);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(2, 0, 0)), 0.80360949039459228516);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(0, 1, 0)), 1.2491617202758789062);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(1, 1, 0)), 2.8053097724914550781);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(2, 1, 0)), -4.166011810302734375);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(0, 2, 0)), 2.4086174964904785156);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(1, 2, 0)), -0.86411559581756591797);
  EXPECT_FLOAT_EQ(static_cast<float>(err_signal.At(2, 2, 0)), -3.5623354911804199219);
}

TYPED_TEST(Convolution1DTest, graph_forward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input(i_ic, i_i, 0) = static_cast<DataType>(i_i + 1);
    }
  }

  // Evaluate
  fetch::ml::Graph<TypeParam> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution1D<TypeParam>>(
      "Convolution1D", {"Input"}, output_channels, input_channels, kernel_height, stride_size);
  g.SetInput("Input", input);

  TypeParam prediction = g.Evaluate("Convolution1D", true);

  // test correct values
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 1);
  ASSERT_EQ(prediction.shape()[2], 1);

  TensorType gt({output_channels, output_height, 1});
  gt(0, 0, 0) = static_cast<DataType>(-4.28031352977);
  gt(1, 0, 0) = static_cast<DataType>(-4.03654768132);
  gt(2, 0, 0) = static_cast<DataType>(8.11192789580);
  gt(3, 0, 0) = static_cast<DataType>(1.763717529829592);
  gt(4, 0, 0) = static_cast<DataType>(-1.8677866039798);

  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, getStateDict)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Initialise weights
  fetch::ml::layers::Convolution1D<TensorType> conv(
      output_channels, input_channels, kernel_height, stride_size,
      fetch::ml::details::ActivationType::NOTHING, "ConvTest");
  fetch::ml::StateDict<TensorType> sd = conv.StateDict();

  // Get weights
  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);
  auto weights_ptr = sd.dict_["ConvTest_Weights"].weights_;

  // Test correct values
  ASSERT_NE(weights_ptr, nullptr);
  EXPECT_EQ(weights_ptr->shape(),
            std::vector<SizeType>({output_channels, input_channels, kernel_height, 1}));

  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(0, 0, 0, 0)), -0.970493f);
  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(1, 1, 1, 0)), 0.55109727f);
  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(4, 2, 2, 0)), -0.97583634f);
}

TYPED_TEST(Convolution1DTest, saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = fetch::math::SizeType;
  using LayerType = fetch::ml::layers::Convolution1D<TypeParam>;
  using SPType    = typename LayerType::SPType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  std::string input_name  = "Conv1D_Input";
  std::string output_name = "Conv1D_Conv1D";

  // Generate input
  TypeParam input(std::vector<typename TypeParam::SizeType>({input_channels, input_height, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input(i_ic, i_i, 0) = static_cast<DataType>(i_i + 1);
    }
  }

  TypeParam labels({output_channels, 1, 1});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(output_channels, input_channels, kernel_height, stride_size);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and evaluate
  layer.SetInput(input_name, input);
  TypeParam prediction;
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
    grad *= static_cast<DataType>(-0.1);
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= static_cast<DataType>(-0.1);
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
