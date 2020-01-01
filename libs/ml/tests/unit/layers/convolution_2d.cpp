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
#include "ml/layers/convolution_2d.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"

#include <memory>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class Convolution2DTest : public ::testing::Test
{
};

TYPED_TEST_CASE(Convolution2DTest, math::test::TensorFloatingTypes);

TYPED_TEST(Convolution2DTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_height, input_width, 1});
  input.FillUniformRandom();

  // Evaluate
  fetch::ml::layers::Convolution2D<TensorType> conv(output_channels, input_channels, kernel_height,
                                                    stride_size);
  conv.SetInput("Conv2D_Input", input);
  TensorType output = conv.Evaluate("Conv2D_Conv2D", true);

  // Get ground truth
  auto                                      weights = conv.GetWeights();
  fetch::ml::ops::Convolution2D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  // Test correct shape and values
  EXPECT_EQ(output.shape(), gt.shape());
  EXPECT_TRUE(output.AllClose(gt, math::function_tolerance<DataType>(),
                              math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution2DTest, ops_forward_test)  // Use the class as an Ops
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_height, input_width, 1});
  input.FillUniformRandom();

  // Evaluate
  fetch::ml::layers::Convolution2D<TensorType> conv(output_channels, input_channels, kernel_height,
                                                    stride_size);

  TensorType output(conv.ComputeOutputShape({std::make_shared<TensorType>(input)}));
  conv.Forward({std::make_shared<TensorType>(input)}, output);

  // Get ground truth
  auto                                      weights = conv.GetWeights();
  fetch::ml::ops::Convolution2D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  // Test correct shape and values
  EXPECT_EQ(output.shape(), gt.shape());
  EXPECT_TRUE(output.AllClose(gt, math::function_tolerance<DataType>(),
                              math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution2DTest, ops_backward_test)  // Use the class as an Ops
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_height, input_width, 1});
  input.FillUniformRandom();

  // Generate error
  TensorType error_signal({output_channels, output_height, output_width, 1});
  error_signal.FillUniformRandom();

  // Evaluate
  fetch::ml::layers::Convolution2D<TensorType> conv(output_channels, input_channels, kernel_height,
                                                    stride_size);

  TensorType output(conv.ComputeOutputShape({std::make_shared<TensorType>(input)}));
  conv.Forward({std::make_shared<TensorType>(input)}, output);

  std::vector<TensorType> backprop_error =
      conv.Backward({std::make_shared<TensorType>(input)}, error_signal);

  // generate ground truth
  auto                                      weights = conv.GetWeights();
  fetch::ml::ops::Convolution2D<TensorType> op;
  std::vector<TensorType>                   gt =
      op.Backward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights[0])},
                  error_signal);

  // test correct shapes and values
  EXPECT_EQ(backprop_error.size(), 1);
  EXPECT_EQ(backprop_error[0].shape(), gt[0].shape());
  EXPECT_TRUE(backprop_error[0].AllClose(gt[0], math::function_tolerance<DataType>(),
                                         math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution2DTest, node_forward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_height, input_width, 1});
  input.FillUniformRandom();

  // Evaluate
  auto placeholder_node = std::make_shared<fetch::ml::Node<TensorType>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TensorType>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TensorType>>(placeholder_node->GetOp())
      ->SetData(input);

  auto conv = fetch::ml::Node<TensorType>(
      fetch::ml::OpType::LAYER_CONVOLUTION_2D, "Convolution2D",
      [output_channels, input_channels, kernel_height, stride_size]() {
        return std::make_shared<fetch::ml::layers::Convolution2D<TensorType>>(
            output_channels, input_channels, kernel_height, stride_size);
      });
  conv.AddInput(placeholder_node);

  TensorType prediction = *conv.Evaluate(true);

  // Get ground truth
  auto weights =
      (std::dynamic_pointer_cast<fetch::ml::layers::Convolution2D<TensorType>>(conv.GetOp()))
          ->GetWeights();
  fetch::ml::ops::Convolution2D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  // Test correct shape and values
  EXPECT_EQ(prediction.shape(), gt.shape());
  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution2DTest, node_backward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_height, input_width, 1});
  input.FillUniformRandom();

  // Generate error
  TensorType error_signal({output_channels, output_height, output_width, 1});
  error_signal.FillUniformRandom();

  // Evaluate
  auto placeholder_node = std::make_shared<fetch::ml::Node<TensorType>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TensorType>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TensorType>>(placeholder_node->GetOp())
      ->SetData(input);

  auto conv2d_layer_ptr = std::make_shared<fetch::ml::layers::Convolution2D<TensorType>>(
      output_channels, input_channels, kernel_height, stride_size);
  auto conv = fetch::ml::Node<TensorType>(
      fetch::ml::OpType::LAYER_CONVOLUTION_2D, "Convolution2D",
      [output_channels, input_channels, kernel_height, stride_size]() {
        return std::make_shared<fetch::ml::layers::Convolution2D<TensorType>>(
            output_channels, input_channels, kernel_height, stride_size);
      });
  conv.AddInput(placeholder_node);
  TensorType prediction     = *conv.Evaluate(true);
  auto       backprop_error = conv.BackPropagate(error_signal);

  // generate ground truth
  auto weights =
      (std::dynamic_pointer_cast<fetch::ml::layers::Convolution2D<TensorType>>(conv.GetOp()))
          ->GetWeights();
  fetch::ml::ops::Convolution2D<TensorType> op;
  std::vector<TensorType>                   gt =
      op.Backward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights[0])},
                  error_signal);

  // test correct shapes and values
  EXPECT_EQ(backprop_error.size(), 1);
  auto err_signal = (*(backprop_error.begin())).second.at(0);
  EXPECT_EQ(err_signal.shape(), gt[0].shape());
  EXPECT_TRUE(err_signal.AllClose(gt[0], math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution2DTest, graph_forward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_height, input_width, 1});
  input.FillUniformRandom();

  // Evaluate
  fetch::ml::Graph<TensorType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution2D<TensorType>>(
      "Convolution2D", {"Input"}, output_channels, input_channels, kernel_height, stride_size);
  g.SetInput("Input", input);

  TensorType prediction = g.Evaluate("Convolution2D", true);

  // Get ground truth
  auto                                      weights = g.GetWeights();
  fetch::ml::ops::Convolution2D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  EXPECT_EQ(prediction.shape(), gt.shape());

  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution2DTest, getStateDict)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Initialise weights
  fetch::ml::layers::Convolution2D<TensorType> conv(
      output_channels, input_channels, kernel_height, stride_size,
      fetch::ml::details::ActivationType::NOTHING, "ConvTest");
  fetch::ml::StateDict<TensorType> sd = conv.StateDict();

  // Get weights
  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);
  auto weights_ptr = sd.dict_["ConvTest_Weights"].weights_;

  // Get ground truth
  TensorType gt_weights = conv.GetWeights()[0];

  // Test correct values
  ASSERT_NE(weights_ptr, nullptr);
  EXPECT_TRUE(weights_ptr->AllClose(gt_weights, math::function_tolerance<DataType>(),
                                    math::function_tolerance<DataType>()));
  EXPECT_EQ(weights_ptr->shape(), std::vector<SizeType>({output_channels, input_channels,
                                                         kernel_height, kernel_height, 1}));
}

TYPED_TEST(Convolution2DTest, saveparams_test)
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

}  // namespace test
}  // namespace ml
}  // namespace fetch
