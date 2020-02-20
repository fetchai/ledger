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

#include "math/fundamental_operators.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/serializers/ml_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class LayerNormTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(LayerNormTest, math::test::TensorFloatingTypes, );

TYPED_TEST(LayerNormTest, set_input_and_evaluate_test_2D)  // Use the class as a subgraph
{
  fetch::ml::layers::LayerNorm<TypeParam> ln({100u, 10u});

  TypeParam input_data(std::vector<typename TypeParam::SizeType>({100, 10, 2}));
  ln.SetInput("LayerNorm_Input", input_data);
  ln.Compile();

  TypeParam output = ln.Evaluate("LayerNorm_Beta_Addition", true);

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 100);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);
}

TYPED_TEST(LayerNormTest, forward_test_1D)  // Use the class as a subgraph
{
  fetch::ml::layers::LayerNorm<TypeParam> ln({100u});
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({100, 2}));
  TypeParam output(ln.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  ln.Forward({std::make_shared<TypeParam>(input_data)}, output);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 100);
  ASSERT_EQ(output.shape()[1], 2);
}

TYPED_TEST(LayerNormTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::LayerNorm<TypeParam> ln({50, 10});
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({50, 10, 2}));

  TypeParam output(ln.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  ln.Forward({std::make_shared<TypeParam>(input_data)}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({50, 10, 2}));

  std::vector<TypeParam> backprop_error =
      ln.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 50);
  ASSERT_EQ(backprop_error[0].shape()[1], 10);
  ASSERT_EQ(backprop_error[0].shape()[2], 2);
}

TYPED_TEST(LayerNormTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  auto      placeholder = std::make_shared<fetch::ml::Node<TypeParam>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder->GetOp())
      ->SetData(data);

  fetch::ml::Node<TypeParam> ln(fetch::ml::OpType::LAYER_LAYER_NORM, "LayerNorm", []() {
    return std::make_shared<fetch::ml::layers::LayerNorm<TypeParam>>(
        std::vector<fetch::math::SizeType>({5, 10}));
  });
  ln.AddInput(placeholder);

  TypeParam prediction = *ln.Evaluate(true);

  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 10);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(LayerNormTest, node_backward_test)  // Use the class as a Node
{
  TypeParam data({5, 10, 2});
  auto      placeholder = std::make_shared<fetch::ml::Node<TypeParam>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder->GetOp())
      ->SetData(data);

  fetch::ml::Node<TypeParam> ln(fetch::ml::OpType::LAYER_LAYER_NORM, "LayerNorm", []() {
    return std::make_shared<fetch::ml::layers::LayerNorm<TypeParam>>(
        std::vector<fetch::math::SizeType>({5, 10}));
  });
  ln.AddInput(placeholder);

  TypeParam prediction = *ln.Evaluate(true);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  auto      backprop_error = ln.BackPropagate(error_signal);

  ASSERT_EQ(backprop_error.size(), 1);
  auto err_signal = (*(backprop_error.begin())).second.at(0);
  ASSERT_EQ(err_signal.shape().size(), 3);
  ASSERT_EQ(err_signal.shape()[0], 5);
  ASSERT_EQ(err_signal.shape()[1], 10);
  ASSERT_EQ(err_signal.shape()[2], 2);
}

TYPED_TEST(LayerNormTest, graph_forward_test_exact_value_2D)  // Use the class as a Node
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::LayerNorm<TypeParam>>(
      "LayerNorm", {"Input"}, std::vector<typename TypeParam::SizeType>({3, 2}));

  TensorType data = TensorType::FromString(
      "1, 2, 3, 0;"
      "2, 3, 2, 1;"
      "3, 6, 4, 13");
  data.Reshape({3, 2, 2});

  TensorType gt = TensorType::FromString(
      "-1.22474487, -0.98058068, 0, -0.79006571;"
      "0, -0.39223227, -1.22474487,  -0.62076591;"
      "1.22474487,  1.37281295, 1.22474487, 1.41083162");
  gt.Reshape({3, 2, 2});

  g.SetInput("Input", data);
  g.Compile();

  TypeParam prediction = g.Evaluate("LayerNorm", true);
  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                          static_cast<DataType>(5) * fetch::math::function_tolerance<DataType>()));
}

// TODO (#1458) enable large dimension test once Add and Multiply layers can handle input of more
// than 3 dims

}  // namespace test
}  // namespace ml
}  // namespace fetch
