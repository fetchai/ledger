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

#include "test_types.hpp"

#include "ml/layers/convolution_1d.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/serialisers/ml_types.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class Convolution1DTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(Convolution1DTest, math::test::TensorFloatingTypes, );

TYPED_TEST(Convolution1DTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_length    = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_length, 1});
  input.FillUniformRandom();

  // Evaluate
  fetch::ml::layers::Convolution1D<TensorType> conv(output_channels, input_channels, kernel_height,
                                                    stride_size);
  conv.SetInput("Conv1D_Input", input);
  conv.Compile();

  TensorType output = conv.Evaluate("Conv1D_Conv1D", true);

  // Get ground truth
  auto                                      weights = conv.GetWeights();
  fetch::ml::ops::Convolution1D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  EXPECT_EQ(output.shape(), gt.shape());

  EXPECT_TRUE(output.AllClose(gt, math::function_tolerance<DataType>(),
                              math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution1DTest, ops_forward_test)  // Use the class as an Ops
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_length    = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  math::SizeVector const input_shape{input_channels, input_length, 1};
  TensorType             input(input_shape);
  input.FillUniformRandom();

  // Evaluate
  fetch::ml::layers::Convolution1D<TensorType> conv(output_channels, input_channels, kernel_height,
                                                    stride_size);

  conv.ComputeBatchOutputShape({input_shape});  // necessary for out-of-Graph usage
  conv.CompleteShapeDeduction();                // necessary for out-of-Graph usage

  TensorType output(conv.ComputeOutputShape({std::make_shared<TensorType>(input)}));
  conv.Forward({std::make_shared<TensorType>(input)}, output);

  // Get ground truth
  auto                                      weights = conv.GetWeights();
  fetch::ml::ops::Convolution1D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  // Test correct shape and values
  EXPECT_EQ(output.shape(), gt.shape());
  EXPECT_TRUE(output.AllClose(gt, math::function_tolerance<DataType>(),
                              math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution1DTest, ops_backward_test)  // Use the class as an Ops
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_length    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  math::SizeVector const input_shape{input_channels, input_length, 1};
  TensorType             input(input_shape);
  input.FillUniformRandom();

  // Generate error
  TensorType error_signal({output_channels, output_height, 1});
  error_signal.FillUniformRandom();

  // Evaluate
  fetch::ml::layers::Convolution1D<TensorType> conv(output_channels, input_channels, kernel_height,
                                                    stride_size);

  conv.ComputeBatchOutputShape({input_shape});  // necessary for out-of-Graph usage
  conv.CompleteShapeDeduction();                // necessary for out-of-Graph usage

  TensorType output(conv.ComputeOutputShape({std::make_shared<TensorType>(input)}));
  conv.Forward({std::make_shared<TensorType>(input)}, output);

  std::vector<TensorType> backprop_error =
      conv.Backward({std::make_shared<TensorType>(input)}, error_signal);

  // generate ground truth
  auto                                      weights = conv.GetWeights();
  fetch::ml::ops::Convolution1D<TensorType> op;
  std::vector<TensorType>                   gt =
      op.Backward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights[0])},
                  error_signal);

  // test correct shapes and values
  EXPECT_EQ(backprop_error.size(), 1);
  EXPECT_EQ(backprop_error[0].shape(), gt[0].shape());
  EXPECT_TRUE(backprop_error[0].AllClose(gt[0], math::function_tolerance<DataType>(),
                                         math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution1DTest, node_forward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_length    = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  math::SizeVector const input_shape{input_channels, input_length, 1};
  TensorType             input(input_shape);
  input.FillUniformRandom();

  // Evaluate
  auto placeholder_node = std::make_shared<fetch::ml::Node<TensorType>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TensorType>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TensorType>>(placeholder_node->GetOp())
      ->SetData(input);

  auto conv = fetch::ml::Node<TensorType>(
      fetch::ml::OpType::LAYER_CONVOLUTION_1D, "Convolution2D",
      [output_channels, input_channels, kernel_height, stride_size]() {
        return std::make_shared<fetch::ml::layers::Convolution1D<TensorType>>(
            output_channels, input_channels, kernel_height, stride_size);
      });
  conv.AddInput(placeholder_node);
  conv.GetOp()->ComputeBatchOutputShape({input_shape});  // necessary for out-of-Graph usage
  conv.GetOp()->CompleteShapeDeduction();                // necessary for out-of-Graph usage

  TensorType prediction = *conv.Evaluate(true);

  // Get ground truth
  auto weights =
      (std::dynamic_pointer_cast<fetch::ml::layers::Convolution1D<TensorType>>(conv.GetOp()))
          ->GetWeights();
  fetch::ml::ops::Convolution1D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  // Test correct shape and values
  EXPECT_EQ(prediction.shape(), gt.shape());
  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution1DTest, node_backward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_length    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  math::SizeVector const input_shape{input_channels, input_length, 1};
  TensorType             input(input_shape);
  input.FillUniformRandom();

  // Generate error
  TensorType error_signal({output_channels, output_height, 1});
  error_signal.FillUniformRandom();

  // Evaluate
  auto placeholder_node = std::make_shared<fetch::ml::Node<TensorType>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TensorType>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TensorType>>(placeholder_node->GetOp())
      ->SetData(input);

  auto conv = fetch::ml::Node<TensorType>(
      fetch::ml::OpType::LAYER_CONVOLUTION_1D, "Convolution2D",
      [output_channels, input_channels, kernel_height, stride_size]() {
        return std::make_shared<fetch::ml::layers::Convolution1D<TensorType>>(
            output_channels, input_channels, kernel_height, stride_size);
      });
  conv.AddInput(placeholder_node);
  conv.GetOp()->ComputeBatchOutputShape({input_shape});  // necessary for out-of-Graph usage
  conv.GetOp()->CompleteShapeDeduction();                // necessary for out-of-Graph usage

  TensorType prediction     = *conv.Evaluate(true);
  auto       backprop_error = conv.BackPropagate(error_signal);

  // generate ground truth
  auto weights =
      (std::dynamic_pointer_cast<fetch::ml::layers::Convolution1D<TensorType>>(conv.GetOp()))
          ->GetWeights();
  fetch::ml::ops::Convolution1D<TensorType> op;
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

TYPED_TEST(Convolution1DTest, graph_forward_test)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_length    = 3;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Generate input
  TensorType input({input_channels, input_length, 1});
  input.FillUniformRandom();

  // Evaluate
  fetch::ml::Graph<TensorType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      "Convolution1D", {"Input"}, output_channels, input_channels, kernel_height, stride_size);
  g.SetInput("Input", input);
  g.Compile();

  TensorType prediction = g.Evaluate("Convolution1D", true);

  // Get ground truth
  auto                                      weights = g.GetWeights();
  fetch::ml::ops::Convolution1D<TensorType> c;
  TensorType                                gt(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights.at(0))}, gt);

  EXPECT_EQ(prediction.shape(), gt.shape());

  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
