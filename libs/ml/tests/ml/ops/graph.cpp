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

#include "ml/graph.hpp"

#include "math/tensor.hpp"
#include "ml/layers/self_attention.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/subtract.hpp"

#include "gtest/gtest.h"

template <typename T>
class GraphTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(GraphTest, MyTypes);

TYPED_TEST(GraphTest, node_placeholder)
{
  using ArrayType = TypeParam;

  fetch::ml::Graph<ArrayType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});

  ArrayType data = ArrayType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");
  ArrayType gt   = ArrayType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");

  g.SetInput("Input", data);
  ArrayType prediction = g.Evaluate("Input");

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(GraphTest, node_relu)
{
  using ArrayType = TypeParam;

  fetch::ml::Graph<ArrayType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.template AddNode<fetch::ml::ops::Relu<ArrayType>>("Relu", {"Input"});

  ArrayType data =
      ArrayType::FromString(R"(0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16)");
  ArrayType gt = ArrayType::FromString(R"(0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0, 16)");

  g.SetInput("Input", data);
  ArrayType prediction = g.Evaluate("Relu");

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(GraphTest, get_state_dict)
{
  using ArrayType = TypeParam;

  fetch::ml::Graph<ArrayType>     g;
  fetch::ml::StateDict<ArrayType> sd = g.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_TRUE(sd.dict_.empty());
}

TYPED_TEST(GraphTest, no_such_node_test)  // Use the class as a Node
{
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  fetch::ml::Graph<ArrayType> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.template AddNode<fetch::ml::layers::SelfAttention<ArrayType>>("SelfAttention", {"Input"}, 50u,
                                                                  42u, 10u);

  ArrayType data(std::vector<SizeType>({5, 10}));
  g.SetInput("Input", data);

  ASSERT_ANY_THROW(g.Evaluate("FullyConnected"));
}

TYPED_TEST(GraphTest, two_nodes_same_name_test)
{
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  fetch::ml::Graph<ArrayType> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  std::string sa_1 = g.template AddNode<fetch::ml::layers::SelfAttention<ArrayType>>(
      "SelfAttention", {"Input"}, 50u, 42u, 10u);
  std::string sa_2 = g.template AddNode<fetch::ml::layers::SelfAttention<ArrayType>>(
      "SelfAttention", {"Input"}, 50u, 42u, 10u);
  std::string sa_3 = g.template AddNode<fetch::ml::layers::SelfAttention<ArrayType>>(
      "SelfAttention", {"Input"}, 50u, 42u, 10u);

  ArrayType data(std::vector<SizeType>({5, 10}));
  g.SetInput("Input", data);

  EXPECT_NE(sa_1, sa_2);
  EXPECT_NE(sa_2, sa_3);
  EXPECT_NE(sa_1, sa_3);
  EXPECT_EQ(sa_1, "SelfAttention");
  EXPECT_EQ(sa_2, "SelfAttention_0");
  EXPECT_EQ(sa_3, "SelfAttention_1");
}

TYPED_TEST(GraphTest,
           diamond_graph_forward)  // Evaluate graph output=(input1*input2)-(input1^2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  // Generate input
  ArrayType data1 = ArrayType::FromString(R"(-1,0,1,2,3,4)");
  ArrayType data2 = ArrayType::FromString(R"(-20,-10, 0, 10, 20, 30)");
  ArrayType gt    = ArrayType::FromString(R"(19, -0, -1, 16, 51, 104)");

  // Create graph
  std::string                 name = "Diamond";
  fetch::ml::Graph<TypeParam> g;

  std::string input_name1 =
      g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input1", {});

  std::string input_name2 =
      g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input2", {});

  std::string op1_name = g.template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
      name + "_Op1", {input_name1, input_name1});
  std::string op2_name = g.template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
      name + "_Op2", {input_name1, input_name2});

  std::string output_name =
      g.template AddNode<fetch::ml::ops::Subtract<ArrayType>>(name + "_Op3", {op2_name, op1_name});

  // Evaluate

  g.SetInput(input_name1, data1);
  g.SetInput(input_name2, data2);
  TypeParam output = g.Evaluate("Diamond_Op3");

  // Test correct values
  ASSERT_EQ(output.shape(), data1.shape());
  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));

  // Change data2
  data2 = ArrayType::FromString(R"(-2, -1, 0, 1, 2, 3)");
  gt    = ArrayType::FromString(R"(1, -0, -1, -2, -3, -4)");
  g.SetInput(input_name2, data2);

  // Recompute graph
  output = g.Evaluate("Diamond_Op3");

  // Test correct values
  ASSERT_EQ(output.shape(), data1.shape());
  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(GraphTest, diamond_graph_backward)  // output=(input1*input2)-(input1^2)
{
  using DataType = typename TypeParam::Type;
  //  using SizeType  = typename TypeParam::SizeType;
  using ArrayType = TypeParam;

  // Generate input
  ArrayType data1        = ArrayType::FromString(R"(-1,0,1,2,3,4)");
  ArrayType data2        = ArrayType::FromString(R"(-20,-10, 0, 10, 20, 30)");
  ArrayType error_signal = ArrayType::FromString(R"(-1,0,1,2,3,4)");
  ArrayType grad1        = ArrayType::FromString(R"(1,  0,  1,  4,  9, 16)");
  ArrayType grad2        = ArrayType::FromString(R"(18, 0, -2, 12, 42, 88)");

  // Create graph
  std::string                 name = "Diamond";
  fetch::ml::Graph<TypeParam> g;

  std::string input_name1 =
      g.template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Input1", {});

  std::string input_name2 =
      g.template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Input2", {});

  std::string op1_name = g.template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
      name + "_Op1", {input_name1, input_name1});
  std::string op2_name = g.template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
      name + "_Op2", {input_name1, input_name2});

  std::string output_name =
      g.template AddNode<fetch::ml::ops::Subtract<ArrayType>>(name + "_Op3", {op2_name, op1_name});

  // Forward
  g.SetInput(input_name1, data1);
  g.SetInput(input_name2, data2);
  TypeParam output = g.Evaluate(output_name);

  // Calculate Gradient
  g.BackPropagate(output_name, error_signal);

  // Test gradient
  std::vector<TypeParam> gradients = g.GetGradients();
  EXPECT_EQ(gradients.size(), 2);
  ASSERT_TRUE(
      gradients[1].AllClose(grad1, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
  ASSERT_TRUE(
      gradients[0].AllClose(grad2, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));

  // Test Weights
  std::vector<TypeParam> weights = g.get_weights();
  EXPECT_EQ(weights.size(), 2);
  ASSERT_TRUE(
      weights[1].AllClose(data2, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
  ASSERT_TRUE(
      weights[0].AllClose(data1, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));

  // Change data2
  data2                       = ArrayType::FromString(R"(-2, -1, 0, 1, 2, 3)");
  error_signal                = ArrayType::FromString(R"(-0.1,0,0.1,0.2,0.3,0.4)");
  ArrayType weights1_expected = ArrayType::FromString(R"(-1,-1,1,5,11,19)");
  ArrayType weights2_expected = ArrayType::FromString(R"(17, 0, -1, 14, 45, 92)");
  grad1                       = ArrayType::FromString(R"(-1.7,0,-0.1,2.8,13.5,36.8)");
  grad2                       = ArrayType::FromString(R"(3.5, 0, 0.3, -4.6, -23.7, -66)");

  g.SetInput(input_name2, data2);

  // Apply gradient;
  g.ApplyGradients(gradients);

  // Recompute graph
  output = g.Evaluate("Diamond_Op3");

  // Calculate Gradient
  g.BackPropagate(output_name, error_signal);

  // Test Weights
  std::vector<TypeParam> weights2 = g.get_weights();
  EXPECT_EQ(weights2.size(), 2);
  ASSERT_TRUE(weights2[1].AllClose(weights1_expected, static_cast<DataType>(1e-5f),
                                   static_cast<DataType>(1e-5f)));
  ASSERT_TRUE(weights2[0].AllClose(weights2_expected, static_cast<DataType>(1e-5f),
                                   static_cast<DataType>(1e-5f)));

  // Test gradient
  std::vector<TypeParam> gradients2 = g.GetGradients();
  EXPECT_EQ(gradients2.size(), 2);
  ASSERT_TRUE(
      gradients2[1].AllClose(grad1, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
  ASSERT_TRUE(
      gradients2[0].AllClose(grad2, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(GraphTest, diamond_graph_getStateDict)
{
  using ArrayType = TypeParam;

  // Generate input
  ArrayType data1 = ArrayType::FromString(R"(-1,0,1,2,3,4)");
  ArrayType data2 = ArrayType::FromString(R"(-20,-10, 0, 10, 20, 30)");

  // Create graph
  std::string                 name = "Diamond";
  fetch::ml::Graph<TypeParam> g;

  std::string input_name1 =
      g.template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Weight1", {});

  std::string input_name2 =
      g.template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Weight2", {});

  std::string op1_name = g.template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
      name + "_Op1", {input_name1, input_name1});
  std::string op2_name = g.template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
      name + "_Op2", {input_name1, input_name2});

  std::string output_name =
      g.template AddNode<fetch::ml::ops::Subtract<ArrayType>>(name + "_Op3", {op2_name, op1_name});

  g.SetInput(input_name1, data1);
  g.SetInput(input_name2, data2);

  // Get statedict
  fetch::ml::StateDict<TypeParam> sd = g.StateDict();

  // Test weights
  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 2);

  ASSERT_NE(sd.dict_["Diamond_Weight1"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["Diamond_Weight1"].weights_->shape(), data1.shape());

  ASSERT_NE(sd.dict_["Diamond_Weight2"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["Diamond_Weight2"].weights_->shape(), data2.shape());
}
