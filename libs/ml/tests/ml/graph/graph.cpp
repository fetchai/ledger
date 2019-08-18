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
#include "ml/core/graph.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/fully_connected.hpp"
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
                                 fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                 fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

TYPED_TEST_CASE(GraphTest, MyTypes);

TYPED_TEST(GraphTest, node_placeholder)
{
  using TensorType = TypeParam;

  fetch::ml::Graph<TensorType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});

  TensorType data = TensorType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");
  TensorType gt   = TensorType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");

  g.SetInput("Input", data);
  TensorType prediction = g.ForwardPropagate("Input");

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(GraphTest, node_relu)
{
  using TensorType = TypeParam;

  fetch::ml::Graph<TensorType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  g.template AddNode<fetch::ml::ops::Relu<TensorType>>("Relu", {"Input"});

  TensorType data =
      TensorType::FromString(R"(0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16)");
  TensorType gt =
      TensorType::FromString(R"(0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0, 16)");

  g.SetInput("Input", data);
  TensorType prediction = g.ForwardPropagate("Relu");

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(GraphTest, get_state_dict)
{
  using TensorType = TypeParam;

  fetch::ml::Graph<TensorType>     g;
  fetch::ml::StateDict<TensorType> sd = g.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_TRUE(sd.dict_.empty());
}

TYPED_TEST(GraphTest, no_such_node_test)  // Use the class as a Node
{
  using TensorType = TypeParam;
  using SizeType   = typename TypeParam::SizeType;

  fetch::ml::Graph<TensorType> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution1D<TensorType>>("Convolution1D", {"Input"}, 3u,
                                                                   3u, 3u, 3u);

  TensorType data(std::vector<SizeType>({5, 10}));
  g.SetInput("Input", data);

  ASSERT_ANY_THROW(g.ForwardPropagate("FullyConnected"));
}

TYPED_TEST(GraphTest, multi_nodes_have_same_name)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  fetch::ml::Graph<TensorType> g;

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string fc_1  = g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {input}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      fetch::ml::RegularisationType::NONE, static_cast<DataType>(0));
  std::string fc_2 = g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {fc_1}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      fetch::ml::RegularisationType::NONE, static_cast<DataType>(0));
  std::string fc_3 = g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {fc_2}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      fetch::ml::RegularisationType::NONE, static_cast<DataType>(0));

  // check the naming is correct
  ASSERT_EQ(fc_1, "FC1");
  ASSERT_EQ(fc_2, "FC1_Copy_1");
  ASSERT_EQ(fc_3, "FC1_Copy_2");
}

TYPED_TEST(GraphTest,
           diamond_graph_forward)  // Evaluate graph output=(input1*input2)-(input1^2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  // Generate input
  TensorType data1 = TensorType::FromString(R"(-1,0,1,2,3,4)");
  TensorType data2 = TensorType::FromString(R"(-20,-10, 0, 10, 20, 30)");
  TensorType gt    = TensorType::FromString(R"(19, -0, -1, 16, 51, 104)");

  // Create graph
  std::string                 name = "Diamond";
  fetch::ml::Graph<TypeParam> g;

  std::string input_name1 =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input1", {});

  std::string input_name2 =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input2", {});

  std::string op1_name = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      name + "_Op1", {input_name1, input_name1});
  std::string op2_name = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      name + "_Op2", {input_name1, input_name2});

  std::string output_name =
      g.template AddNode<fetch::ml::ops::Subtract<TensorType>>(name + "_Op3", {op2_name, op1_name});

  // Evaluate

  g.SetInput(input_name1, data1);
  g.SetInput(input_name2, data2);
  TypeParam output = g.ForwardPropagate("Diamond_Op3");

  // Test correct values
  ASSERT_EQ(output.shape(), data1.shape());
  ASSERT_TRUE(output.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));

  // Change data2
  data2 = TensorType::FromString(R"(-2, -1, 0, 1, 2, 3)");
  gt    = TensorType::FromString(R"(1, -0, -1, -2, -3, -4)");
  g.SetInput(input_name2, data2);

  // Recompute graph
  output = g.ForwardPropagate("Diamond_Op3");

  // Test correct values
  ASSERT_EQ(output.shape(), data1.shape());
  ASSERT_TRUE(output.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, diamond_graph_backward)  // output=(input1*input2)-(input1^2)
{
  using DataType = typename TypeParam::Type;
  //  using SizeType  = typename TypeParam::SizeType;
  using TensorType = TypeParam;

  // Generate input
  TensorType data1        = TensorType::FromString(R"(-1,0,1,2,3,4)");
  TensorType data2        = TensorType::FromString(R"(-20,-10, 0, 10, 20, 30)");
  TensorType error_signal = TensorType::FromString(R"(-1,0,1,2,3,4)");
  TensorType grad1        = TensorType::FromString(R"(1,  0,  1,  4,  9, 16)");
  TensorType grad2        = TensorType::FromString(R"(18, 0, -2, 12, 42, 88)");

  // Create graph
  std::string                 name = "Diamond";
  fetch::ml::Graph<TypeParam> g;

  std::string input_name1 =
      g.template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Input1", {});

  std::string input_name2 =
      g.template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Input2", {});

  std::string op1_name = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      name + "_Op1", {input_name1, input_name1});
  std::string op2_name = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      name + "_Op2", {input_name1, input_name2});

  std::string output_name =
      g.template AddNode<fetch::ml::ops::Subtract<TensorType>>(name + "_Op3", {op2_name, op1_name});

  // Forward
  g.SetInput(input_name1, data1);
  g.SetInput(input_name2, data2);
  TypeParam output = g.ForwardPropagate(output_name);

  // Calculate Gradient
  g.BackPropagateSignal(output_name, error_signal);

  // Test gradient
  std::vector<TypeParam> gradients = g.GetGradients();
  EXPECT_EQ(gradients.size(), 2);
  ASSERT_TRUE(gradients[1].AllClose(grad1, fetch::math::function_tolerance<DataType>(),
                                    fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(gradients[0].AllClose(grad2, fetch::math::function_tolerance<DataType>(),
                                    fetch::math::function_tolerance<DataType>()));

  // Test Weights
  std::vector<TypeParam> weights = g.get_weights();
  EXPECT_EQ(weights.size(), 2);
  ASSERT_TRUE(weights[1].AllClose(data2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights[0].AllClose(data1, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // Change data2
  data2                        = TensorType::FromString(R"(-2, -1, 0, 1, 2, 3)");
  error_signal                 = TensorType::FromString(R"(-0.1,0,0.1,0.2,0.3,0.4)");
  TensorType weights1_expected = TensorType::FromString(R"(-1,-1,1,5,11,19)");
  TensorType weights2_expected = TensorType::FromString(R"(17, 0, -1, 14, 45, 92)");
  grad1                        = TensorType::FromString(R"(-1.7,0,-0.1,2.8,13.5,36.8)");
  grad2                        = TensorType::FromString(R"(3.5, 0, 0.3, -4.6, -23.7, -66)");

  g.SetInput(input_name2, data2);

  // Apply gradient;
  g.ApplyGradients(gradients);

  // Recompute graph
  output = g.ForwardPropagate("Diamond_Op3");

  // Calculate Gradient
  g.BackPropagateSignal(output_name, error_signal);

  // Test Weights
  std::vector<TypeParam> weights2 = g.get_weights();
  EXPECT_EQ(weights2.size(), 2);
  ASSERT_TRUE(weights2[1].AllClose(weights1_expected, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights2[0].AllClose(weights2_expected, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  // Test gradient
  std::vector<TypeParam> gradients2 = g.GetGradients();
  EXPECT_EQ(gradients2.size(), 2);
  ASSERT_TRUE(gradients2[1].AllClose(grad1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(gradients2[0].AllClose(grad2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, diamond_graph_getStateDict)
{
  using TensorType = TypeParam;

  // Generate input
  TensorType data1 = TensorType::FromString(R"(-1,0,1,2,3,4)");
  TensorType data2 = TensorType::FromString(R"(-20,-10, 0, 10, 20, 30)");

  // Create graph
  std::string                 name = "Diamond";
  fetch::ml::Graph<TypeParam> g;

  std::string input_name1 =
      g.template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weight1", {});

  std::string input_name2 =
      g.template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weight2", {});

  std::string op1_name = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      name + "_Op1", {input_name1, input_name1});
  std::string op2_name = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      name + "_Op2", {input_name1, input_name2});

  std::string output_name =
      g.template AddNode<fetch::ml::ops::Subtract<TensorType>>(name + "_Op3", {op2_name, op1_name});

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
