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

#include "gtest/gtest.h"
#include "ml/layers/PRelu.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class PReluTest : public ::testing::Test
{
};

TYPED_TEST_CASE(PReluTest, math::test::TensorFloatingTypes);

TYPED_TEST(PReluTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::PRelu<TypeParam> fc(100u);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({10, 10, 2}));
  fc.SetInput("PRelu_Input", input_data);
  TypeParam output = fc.Evaluate("PRelu_PReluOp", true);

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);

  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::layers::PRelu<TypeParam> pr(50);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  TypeParam output(pr.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  pr.Forward({std::make_shared<TypeParam>(input_data)}, output);

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::PRelu<TypeParam> fc(50);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  TypeParam output(fc.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  fc.Forward({std::make_shared<TypeParam>(input_data)}, output);
  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({50, 2}));

  std::vector<TypeParam> bp_err =
      fc.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);
  ASSERT_EQ(bp_err.size(), 1);
  ASSERT_EQ(bp_err[0].shape().size(), 3);
  ASSERT_EQ(bp_err[0].shape()[0], 5);
  ASSERT_EQ(bp_err[0].shape()[1], 10);
  ASSERT_EQ(bp_err[0].shape()[2], 2);

  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, node_forward_test)  // Use the class as a Node
{
  math::SizeType input_dim_0 = 5;
  math::SizeType input_dim_1 = 10;
  math::SizeType input_dim_2 = 2;

  TypeParam data({input_dim_0, input_dim_1, input_dim_2});
  auto      placeholder_node = std::make_shared<fetch::ml::Node<TypeParam>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  auto placeholder_op_ptr =
      std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder_node->GetOp());
  placeholder_op_ptr->SetData(data);

  fetch::math::SizeType in_size = input_dim_0 * input_dim_1;
  auto                  prelu_node =
      fetch::ml::Node<TypeParam>(fetch::ml::OpType::LAYER_PRELU, "PRelu", [in_size]() {
        return std::make_shared<fetch::ml::layers::PRelu<TypeParam>>(in_size, "PRelu");
      });
  prelu_node.AddInput(placeholder_node);
  auto prediction = prelu_node.Evaluate(true);

  ASSERT_EQ(prediction->shape().size(), 3);
  ASSERT_EQ(prediction->shape()[0], input_dim_0);
  ASSERT_EQ(prediction->shape()[1], input_dim_1);
  ASSERT_EQ(prediction->shape()[2], input_dim_2);
}

TYPED_TEST(PReluTest, node_backward_test)  // Use the class as a Node
{
  math::SizeType input_dim_0 = 5;
  math::SizeType input_dim_1 = 10;
  math::SizeType input_dim_2 = 2;

  TypeParam data({input_dim_0, input_dim_1, input_dim_2});
  auto      placeholder_node = std::make_shared<fetch::ml::Node<TypeParam>>(
      fetch::ml::OpType::OP_PLACEHOLDER, "Input",
      []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  auto placeholder_op_ptr =
      std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder_node->GetOp());
  placeholder_op_ptr->SetData(data);

  fetch::math::SizeType in_size    = input_dim_0 * input_dim_1;
  auto                  prelu_node = fetch::ml::Node<TypeParam>(
      fetch::ml::OpType::LAYER_PRELU, "PRelu",
      [in_size]() { return std::make_shared<fetch::ml::layers::PRelu<TypeParam>>(in_size); });
  prelu_node.AddInput(placeholder_node);
  TypeParam prediction = *prelu_node.Evaluate(true);

  TypeParam error_signal({input_dim_0, input_dim_1, input_dim_2});
  auto      bp_err = prelu_node.BackPropagate(error_signal);

  ASSERT_EQ(bp_err.size(), 1);
  auto err_signal = (*(bp_err.begin())).second.at(0);
  ASSERT_EQ(err_signal.shape().size(), 3);
  ASSERT_EQ(err_signal.shape()[0], input_dim_0);
  ASSERT_EQ(err_signal.shape()[1], input_dim_1);
  ASSERT_EQ(err_signal.shape()[2], input_dim_2);
}

TYPED_TEST(PReluTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  math::SizeType input_dim_0 = 5;
  math::SizeType input_dim_1 = 10;
  math::SizeType input_dim_2 = 2;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::PRelu<TypeParam>>("PRelu", {"Input"},
                                                          input_dim_0 * input_dim_1);

  TypeParam data({input_dim_0, input_dim_1, input_dim_2});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("PRelu", true);
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], input_dim_0);
  ASSERT_EQ(prediction.shape()[1], input_dim_1);
  ASSERT_EQ(prediction.shape()[2], input_dim_2);
}

TYPED_TEST(PReluTest, getStateDict)
{
  fetch::ml::layers::PRelu<TypeParam> fc(50, "PReluTest");
  fetch::ml::StateDict<TypeParam>     sd = fc.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);

  ASSERT_NE(sd.dict_["PReluTest_Alpha"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["PReluTest_Alpha"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({50, 1}));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
