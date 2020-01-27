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

#include "math/tensor/tensor.hpp"
#include "ml/charge_estimation/ops/constants.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/regularisers/l1_regulariser.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class GraphTest : public ::testing::Test
{
};

TYPED_TEST_CASE(GraphTest, math::test::TensorFloatingTypes);

template <class TensorType>
std::shared_ptr<fetch::ml::Graph<TensorType>> MakeGraph()
{
  // Create graph
  std::shared_ptr<fetch::ml::Graph<TensorType>> g =
      std::make_shared<fetch::ml::Graph<TensorType>>();

  std::string input = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});

  std::string layer_1 = g->template AddNode<layers::FullyConnected<TensorType>>(
      "FC1", {input}, 28u * 28u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->template AddNode<layers::FullyConnected<TensorType>>(
      "FC2", {layer_1}, 10u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string output = g->template AddNode<layers::FullyConnected<TensorType>>(
      "FC3", {layer_2}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX);

  return g;
}

TYPED_TEST(GraphTest, node_placeholder)
{
  using TensorType = TypeParam;

  fetch::ml::Graph<TensorType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});

  TensorType data = TensorType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");
  TensorType gt   = TensorType::FromString(R"(1, 2, 3, 4, 5, 6, 7, 8)");

  g.SetInput("Input", data);
  TensorType prediction = g.Evaluate("Input");

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
  TensorType prediction = g.Evaluate("Relu");

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(GraphTest, no_such_node_test)  // Use the class as a Node
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  fetch::ml::Graph<TensorType> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution1D<TensorType>>("Convolution1D", {"Input"}, 3u,
                                                                   3u, 3u, 3u);

  TensorType data(std::vector<SizeType>({5, 10}));
  g.SetInput("Input", data);

  ASSERT_ANY_THROW(g.Evaluate("FullyConnected"));
}

TYPED_TEST(GraphTest, node_add_wrong_order_test)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  fetch::ml::Graph<TensorType> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC1", {"Input"}, 3u, 3u);
  g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC2", {"FC1"}, 3u, 3u);
  g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC3", {"FC2"}, 3u, 3u);

  TensorType data(std::vector<SizeType>({3, 10}));
  g.SetInput("Input", data);

  auto result = g.Evaluate("FC3");

  fetch::ml::Graph<TensorType> g2;

  g2.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC3", {"FC2"}, 3u, 3u);
  g2.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC2", {"FC1"}, 3u, 3u);
  g2.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC1", {"Input"}, 3u, 3u);
  g2.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});

  TensorType data2(std::vector<SizeType>({3, 10}));
  g2.SetInput("Input", data);

  auto result2 = g2.Evaluate("FC3");

  EXPECT_TRUE(result == result2);
}

TYPED_TEST(GraphTest, multi_nodes_have_same_name)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  fetch::ml::Graph<TensorType> g;

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string fc_1  = g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {input}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      fetch::ml::RegularisationType::NONE, DataType{0});
  std::string fc_2 = g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {fc_1}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      fetch::ml::RegularisationType::NONE, DataType{0});
  std::string fc_3 = g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {fc_2}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      fetch::ml::RegularisationType::NONE, DataType{0});

  // check the naming is correct
  ASSERT_EQ(fc_1, "FC1");
  ASSERT_EQ(fc_2, "FC1_Copy_1");
  ASSERT_EQ(fc_3, "FC1_Copy_2");
}

TYPED_TEST(GraphTest, applying_regularisation_per_trainable)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using RegType    = fetch::ml::regularisers::L1Regulariser<TensorType>;

  // Initialise values
  auto regularisation_rate = fetch::math::Type<DataType>("0.1");
  auto regulariser         = std::make_shared<RegType>();

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString("0.9, -1.9, 2.9, -3.9, 4.9, -5.9, 6.9, -7.9");

  // Create a graph with a single weights node
  fetch::ml::Graph<TensorType> g;

  std::string weights = g.template AddNode<fetch::ml::ops::Weights<TensorType>>("Weights", {});

  g.SetInput(weights, data);

  // Apply regularisation
  g.SetRegularisation(weights, regulariser, regularisation_rate);
  auto node_ptr = g.GetNode(weights);
  auto op_ptr   = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(node_ptr->GetOp());
  TensorType grad = op_ptr->GetGradients();
  grad.Fill(DataType{0});
  op_ptr->ApplyGradient(grad);

  // Evaluate weights
  TensorType prediction(op_ptr->ComputeOutputShape({}));
  op_ptr->Forward({}, prediction);

  // Test actual values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, applying_regularisation_all_trainables)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using RegType    = fetch::ml::regularisers::L1Regulariser<TensorType>;

  // Initialise values
  auto regularisation_rate = fetch::math::Type<DataType>("0.1");
  auto regulariser         = std::make_shared<RegType>();

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString("0.9, -1.9, 2.9, -3.9, 4.9, -5.9, 6.9, -7.9");

  // Create a graph with a single weights node
  fetch::ml::Graph<TensorType> g;

  std::string weights = g.template AddNode<fetch::ml::ops::Weights<TensorType>>("Weights", {});

  g.SetInput(weights, data);

  // Apply regularisation to all trainables
  g.SetRegularisation(regulariser, regularisation_rate);
  auto node_ptr = g.GetNode(weights);
  auto op_ptr   = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(node_ptr->GetOp());
  TensorType grad = op_ptr->GetGradients();
  grad.Fill(DataType{0});
  op_ptr->ApplyGradient(grad);

  // Evaluate weights
  TensorType prediction(op_ptr->ComputeOutputShape({}));
  op_ptr->Forward({}, prediction);

  // Test actual values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, variable_freezing_per_trainable)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType data_2 = TensorType::FromString("3, 0, 5, -2, 7, -4, 9, -6");

  // Create a graph with a single weights node
  fetch::ml::Graph<TensorType> g;

  std::string weights = g.template AddNode<fetch::ml::ops::Weights<TensorType>>("Weights", {});

  g.SetInput(weights, data_1);

  // Freeze variable
  g.SetFrozenState(weights, true);
  auto node_ptr = g.GetNode(weights);
  auto op_ptr   = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(node_ptr->GetOp());

  // Apply gradient
  TensorType grad = op_ptr->GetGradients();
  grad.Fill(DataType{2});
  op_ptr->ApplyGradient(grad);

  // Evaluate weights
  TensorType prediction_1(op_ptr->ComputeOutputShape({}));
  op_ptr->Forward({}, prediction_1);

  // Test if weights didn't change
  ASSERT_TRUE(prediction_1.AllClose(data_1, fetch::math::function_tolerance<DataType>(),
                                    fetch::math::function_tolerance<DataType>()));

  // Un-freeze variable
  g.SetFrozenState(false);
  op_ptr->ApplyGradient(grad);

  // Evaluate weights
  TensorType prediction_2(op_ptr->ComputeOutputShape({}));
  op_ptr->Forward({}, prediction_2);

  // Test actual values
  ASSERT_TRUE(prediction_2.AllClose(data_2, fetch::math::function_tolerance<DataType>(),
                                    fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, variable_freezing_all_trainables)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType data_2 = TensorType::FromString("3, 0, 5, -2, 7, -4, 9, -6");

  // Create a graph with a single weights node
  fetch::ml::Graph<TensorType> g;

  std::string weights = g.template AddNode<fetch::ml::ops::Weights<TensorType>>("Weights", {});

  g.SetInput(weights, data_1);

  // Freeze variable
  g.SetFrozenState(true);
  auto node_ptr = g.GetNode(weights);
  auto op_ptr   = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(node_ptr->GetOp());
  TensorType grad = op_ptr->GetGradients();
  grad.Fill(DataType{2});
  op_ptr->ApplyGradient(grad);

  // Evaluate weights
  TensorType prediction_1(op_ptr->ComputeOutputShape({}));
  op_ptr->Forward({}, prediction_1);

  // Test actual values
  ASSERT_TRUE(prediction_1.AllClose(data_1, fetch::math::function_tolerance<DataType>(),
                                    fetch::math::function_tolerance<DataType>()));

  // Un-freeze variable
  g.SetFrozenState(false);
  op_ptr->ApplyGradient(grad);

  // Evaluate weights
  TensorType prediction_2(op_ptr->ComputeOutputShape({}));
  op_ptr->Forward({}, prediction_2);

  // Test actual values
  ASSERT_TRUE(prediction_2.AllClose(data_2, fetch::math::function_tolerance<DataType>(),
                                    fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, variable_freezing_subgraph)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  // Dummy values
  TensorType data = TensorType::FromString("1; -2; 3");
  TensorType gt   = TensorType::FromString("1; -2; 3");

  // Create a graph
  fetch::ml::Graph<TensorType> g;

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});
  std::string layer_1 =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC1", {"Input"}, 3u, 3u);
  std::string layer_2 =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC2", {"FC1"}, 3u, 3u);
  std::string layer_3 =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC3", {"FC2"}, 3u, 3u);

  // Add loss function
  std::string error_output = g.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {layer_3, label});

  g.Compile();

  // Calculate Gradient
  g.SetInput(input, data);
  g.SetInput(label, gt);
  TypeParam output = g.Evaluate(error_output);
  g.BackPropagate(error_output);

  // Freeze variables
  g.SetFrozenState(layer_1, true);
  g.SetFrozenState(layer_3, true);

  // Get weights before applying gradient
  auto weights_1 = g.GetWeights();

  // Apply gradient
  auto gradient_vector = g.GetGradients();
  for (auto grad : gradient_vector)
  {
    grad.Fill(DataType{2});
  }
  g.ApplyGradients(gradient_vector);

  // Get weights after applying gradient
  auto weights_2 = g.GetWeights();

  ASSERT_TRUE(weights_1.at(0).AllClose(weights_2.at(0), DataType{0}, DataType{0}));
  ASSERT_TRUE(weights_1.at(1).AllClose(weights_2.at(1), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_1.at(2).AllClose(weights_2.at(2), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_1.at(3).AllClose(weights_2.at(3), DataType{0}, DataType{0}));
  ASSERT_TRUE(weights_1.at(4).AllClose(weights_2.at(4), DataType{0}, DataType{0}));
  ASSERT_TRUE(weights_1.at(5).AllClose(weights_2.at(5), DataType{0}, DataType{0}));

  // Un-freeze variables
  g.SetFrozenState(layer_1, false);
  g.SetFrozenState(layer_3, false);

  // Apply gradient
  auto gradient_vector_2 = g.GetGradients();
  for (auto grad : gradient_vector_2)
  {
    grad.Fill(DataType{2});
  }
  g.ApplyGradients(gradient_vector);

  auto weights_3 = g.GetWeights();

  ASSERT_FALSE(weights_2.at(0).AllClose(weights_3.at(0), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(1).AllClose(weights_3.at(1), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(2).AllClose(weights_3.at(2), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(3).AllClose(weights_3.at(3), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(4).AllClose(weights_3.at(4), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(5).AllClose(weights_3.at(5), DataType{0}, DataType{0}));
}

TYPED_TEST(GraphTest, variable_freezing_shared_layer)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  // Dummy values
  TensorType data = TensorType::FromString("1; -2; 3");
  TensorType gt   = TensorType::FromString("1; -2; 3");

  // Create a graph
  fetch::ml::Graph<TensorType> g;

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});
  std::string layer_1 =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC1", {"Input"}, 3u, 3u);
  std::string layer_2 =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC2", {"FC1"}, 3u, 3u);
  std::string layer_3 =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>("FC1", {"FC2"});

  // Add loss function
  std::string error_output = g.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {layer_3, label});

  g.Compile();

  // Calculate Gradient
  g.SetInput(input, data);
  g.SetInput(label, gt);
  TypeParam output = g.Evaluate(error_output);
  g.BackPropagate(error_output);

  // Freeze variables
  g.SetFrozenState(layer_1, true);

  // Get weights before applying gradient
  auto weights_1 = g.GetWeights();

  // Apply gradient
  auto gradient_vector = g.GetGradients();
  for (auto grad : gradient_vector)
  {
    grad.Fill(DataType{2});
  }
  g.ApplyGradients(gradient_vector);

  // Get weights after applying gradient
  auto weights_2 = g.GetWeights();

  // Test if layer1 and copy of layer1 is frozen
  ASSERT_TRUE(weights_1.at(0).AllClose(weights_2.at(0), DataType{0}, DataType{0}));
  ASSERT_TRUE(weights_1.at(1).AllClose(weights_2.at(1), DataType{0}, DataType{0}));
  ASSERT_TRUE(weights_1.at(2).AllClose(weights_2.at(2), DataType{0}, DataType{0}));
  ASSERT_TRUE(weights_1.at(3).AllClose(weights_2.at(3), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_1.at(4).AllClose(weights_2.at(4), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_1.at(5).AllClose(weights_2.at(5), DataType{0}, DataType{0}));

  // Un-freeze variables
  g.SetFrozenState(layer_1, false);

  // Apply gradient
  auto gradient_vector_2 = g.GetGradients();
  for (auto grad : gradient_vector_2)
  {
    grad.Fill(DataType{2});
  }
  g.ApplyGradients(gradient_vector);

  auto weights_3 = g.GetWeights();

  // Test if everything is unfrozen
  ASSERT_FALSE(weights_2.at(0).AllClose(weights_3.at(0), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(1).AllClose(weights_3.at(1), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(2).AllClose(weights_3.at(2), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(3).AllClose(weights_3.at(3), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(4).AllClose(weights_3.at(4), DataType{0}, DataType{0}));
  ASSERT_FALSE(weights_2.at(5).AllClose(weights_3.at(5), DataType{0}, DataType{0}));
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
  TypeParam output = g.Evaluate("Diamond_Op3");

  // Test correct values
  ASSERT_EQ(output.shape(), data1.shape());
  ASSERT_TRUE(output.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));

  // Change data2
  data2 = TensorType::FromString(R"(-2, -1, 0, 1, 2, 3)");
  gt    = TensorType::FromString(R"(1, -0, -1, -2, -3, -4)");
  g.SetInput(input_name2, data2);

  // Recompute graph
  output = g.Evaluate("Diamond_Op3");

  // Test correct values
  ASSERT_EQ(output.shape(), data1.shape());
  ASSERT_TRUE(output.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, diamond_graph_backward)  // output=(input1*input2)-(input1^2)
{
  using DataType   = typename TypeParam::Type;
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
  TypeParam output = g.Evaluate(output_name);

  // Calculate Gradient
  g.BackPropagate(output_name, error_signal);

  // Test gradient
  std::vector<TypeParam> gradients = g.GetGradients();

  EXPECT_EQ(gradients.size(), 2);
  ASSERT_TRUE((gradients[0].AllClose(grad1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()) &&
               gradients[1].AllClose(grad2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>())) ||
              (gradients[1].AllClose(grad1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()) &&
               gradients[0].AllClose(grad2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>())));

  // Test Weights
  std::vector<TypeParam> weights = g.GetWeights();
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
  output = g.Evaluate("Diamond_Op3");

  // Calculate Gradient
  g.BackPropagate(output_name, error_signal);

  // Test Weights
  std::vector<TypeParam> weights2 = g.GetWeights();
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

TYPED_TEST(GraphTest, compute_shapes_single_placeholder)
{
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");
  math::SizeVector batch_shape = data.shape();
  batch_shape.back()           = 1;  // Because default batch size is always 1.

  fetch::ml::Graph<TensorType> g;

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});

  g.SetInput(input, data);
  g.Compile();

  math::SizeVector const out_shape = g.GetNode(input)->BatchOutputShape();

  ASSERT_EQ(batch_shape, out_shape);
}

TYPED_TEST(GraphTest, compute_shapes_dense_layers)
{
  using TensorType = TypeParam;
  using Dense      = fetch::ml::layers::FullyConnected<TensorType>;

  static constexpr math::SizeType FIRST_LAYER_OUTPUTS  = 3;
  static constexpr math::SizeType SECOND_LAYER_OUTPUTS = 13;
  static constexpr math::SizeType THIRD_LAYER_OUTPUTS  = 9;

  TensorType data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");
  math::SizeVector batch_shape = data.shape();
  batch_shape.back()           = 1;  // Because default batch size is always 1.

  fetch::ml::Graph<TensorType> g;

  std::string input   = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string layer_1 = g.template AddNode<Dense>("FC1", {"Input"}, Dense::AUTODETECT_INPUTS_COUNT,
                                                  FIRST_LAYER_OUTPUTS);
  std::string layer_2 = g.template AddNode<Dense>("FC2", {"FC1"}, Dense::AUTODETECT_INPUTS_COUNT,
                                                  SECOND_LAYER_OUTPUTS);
  std::string output  = g.template AddNode<Dense>("FC3", {"FC2"}, Dense::AUTODETECT_INPUTS_COUNT,
                                                 THIRD_LAYER_OUTPUTS);

  g.SetInput(input, data);
  g.Compile();

  math::SizeVector const out_shape1 = g.GetNode(layer_1)->BatchOutputShape();
  EXPECT_EQ(out_shape1.size(), batch_shape.size());
  EXPECT_EQ(out_shape1.at(0), FIRST_LAYER_OUTPUTS);

  math::SizeVector const out_shape2 = g.GetNode(layer_2)->BatchOutputShape();
  EXPECT_EQ(out_shape2.size(), batch_shape.size());
  EXPECT_EQ(out_shape2.at(0), SECOND_LAYER_OUTPUTS);

  math::SizeVector const out_shape3 = g.GetNode(output)->BatchOutputShape();
  EXPECT_EQ(out_shape3.size(), batch_shape.size());
  EXPECT_EQ(out_shape3.at(0), THIRD_LAYER_OUTPUTS);

  TensorType const       result = g.Evaluate(output);
  math::SizeVector const expected_out_shape{THIRD_LAYER_OUTPUTS, data.shape().back()};
  EXPECT_EQ(result.shape(), expected_out_shape);
}

TYPED_TEST(GraphTest, compute_shapes_two_outputs)
{
  using TensorType = TypeParam;
  using Dense      = fetch::ml::layers::FullyConnected<TensorType>;

  static constexpr math::SizeType CENTER_OUTPUTS = 21;
  static constexpr math::SizeType LEFT_OUTPUTS   = 13;
  static constexpr math::SizeType RIGHT_OUTPUTS  = 9;

  TensorType data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");

  fetch::ml::Graph<TensorType> g;

  //          ┌───────────────┐
  //          │ input {4, 1}  │
  //          └───────┐───────┘
  //                  │
  //                  ▼
  //          ┌───────────────┐
  //          │ dense {21, 1} │
  //          └───────┐───────┘
  //                  │
  //         ┌────────┴─────────┐
  //         ▼                  ▼
  // ┌───────────────┐  ┌───────────────┐
  // │ dense {13, 1} │  │ dense {9, 1}  │
  // └───────────────┘  └───────────────┘

  std::string left_input =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("LeftInput", {});

  std::string center = g.template AddNode<Dense>("Center", {"LeftInput"},
                                                 Dense::AUTODETECT_INPUTS_COUNT, CENTER_OUTPUTS);

  std::string left_output  = g.template AddNode<Dense>("LeftOutput", {"Center"},
                                                      Dense::AUTODETECT_INPUTS_COUNT, LEFT_OUTPUTS);
  std::string right_output = g.template AddNode<Dense>(
      "RightOutput", {"Center"}, Dense::AUTODETECT_INPUTS_COUNT, RIGHT_OUTPUTS);

  g.SetInput(left_input, data);
  g.Compile();

  math::SizeVector const center_out_batch_shape = g.GetNode(center)->BatchOutputShape();
  EXPECT_EQ(center_out_batch_shape.at(0), CENTER_OUTPUTS);

  math::SizeVector const left_out_batch_shape = g.GetNode(left_output)->BatchOutputShape();
  EXPECT_EQ(left_out_batch_shape.at(0), LEFT_OUTPUTS);

  math::SizeVector const right_out_batch_shape = g.GetNode(right_output)->BatchOutputShape();
  EXPECT_EQ(right_out_batch_shape.at(0), RIGHT_OUTPUTS);

  TensorType const left_result  = g.Evaluate(left_output);
  TensorType const right_result = g.Evaluate(right_output);

  math::SizeVector const expected_left_out_shape{LEFT_OUTPUTS, data.shape().back()};
  EXPECT_EQ(left_result.shape(), expected_left_out_shape);

  math::SizeVector const expected_right_out_shape{RIGHT_OUTPUTS, data.shape().back()};
  EXPECT_EQ(right_result.shape(), expected_right_out_shape);
}

TYPED_TEST(GraphTest, compute_shapes_two_inputs_two_outputs)
{
  using TensorType = TypeParam;
  using Dense      = fetch::ml::layers::FullyConnected<TensorType>;

  static constexpr math::SizeType CENTER_OUTPUTS = 21;
  static constexpr math::SizeType LEFT_OUTPUTS   = 13;
  static constexpr math::SizeType RIGHT_OUTPUTS  = 9;

  TensorType left_data =
      TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");
  TensorType right_data = TensorType::FromString(
      R"(011,022,033,044; 111,122,133,144; 211,222,233,244; 311,322,333,344)");

  fetch::ml::Graph<TensorType> g;

  //{4,1} {4,1}  {4,1} {4,1}
  //  li     ri   (li)  (ri)
  //   |     |      |     |
  //  A_D_D{4,1}   S_U_B{4,1}
  //      |         |
  //    M_U_L_T_I_P_L_Y {??}
  //         |
  //    Dense{21, 1}
  //      |       |
  //    Dense    Dense
  //   {13, 1}  {9, 1}

  std::string left_input =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("LeftInput", {});
  std::string right_input =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("RightInput", {});

  std::string add =
      g.template AddNode<fetch::ml::ops::Add<TensorType>>("AddInputs", {"LeftInput", "RightInput"});

  std::string subtract =
      g.template AddNode<fetch::ml::ops::Add<TensorType>>("SubInputs", {"LeftInput", "RightInput"});

  std::string multiply = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      "Multiply", {"AddInputs", "SubInputs"});

  std::string center = g.template AddNode<Dense>("Center", {"Multiply"},
                                                 Dense::AUTODETECT_INPUTS_COUNT, CENTER_OUTPUTS);

  std::string left_output  = g.template AddNode<Dense>("LeftOutput", {"Center"},
                                                      Dense::AUTODETECT_INPUTS_COUNT, LEFT_OUTPUTS);
  std::string right_output = g.template AddNode<Dense>(
      "RightOutput", {"Center"}, Dense::AUTODETECT_INPUTS_COUNT, RIGHT_OUTPUTS);

  g.SetInput(left_input, left_data);
  g.SetInput(right_input, left_data);
  g.Compile();

  math::SizeVector const center_out_batch_shape = g.GetNode(center)->BatchOutputShape();
  EXPECT_EQ(center_out_batch_shape.at(0), CENTER_OUTPUTS);

  math::SizeVector const left_out_batch_shape = g.GetNode(left_output)->BatchOutputShape();
  EXPECT_EQ(left_out_batch_shape.at(0), LEFT_OUTPUTS);

  math::SizeVector const right_out_batch_shape = g.GetNode(right_output)->BatchOutputShape();
  EXPECT_EQ(right_out_batch_shape.at(0), RIGHT_OUTPUTS);

  TensorType const left_result  = g.Evaluate(left_output);
  TensorType const right_result = g.Evaluate(right_output);

  math::SizeVector const expected_left_out_shape{LEFT_OUTPUTS, left_data.shape().back()};
  EXPECT_EQ(left_result.shape(), expected_left_out_shape);

  math::SizeVector const expected_right_out_shape{RIGHT_OUTPUTS, right_data.shape().back()};
  EXPECT_EQ(right_result.shape(), expected_right_out_shape);
}

// (VH): Disabled because shared Dense layers do not work if created with auto-detected inputs.
TYPED_TEST(GraphTest, compute_shapes_sequential_denses_with_shared_ops)
{
  using TensorType = TypeParam;
  using Dense      = fetch::ml::layers::FullyConnected<TensorType>;

  static constexpr math::SizeType NEURONS = 4;

  TensorType data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");

  fetch::ml::Graph<TensorType> g;

  // Note: all 4 Dense nodes share the same single Op.
  //     {4,1}
  //    i_n_p_u_t
  //       |
  //     Dense
  //    {4, 1}
  //       |
  //     Dense - copy
  //    {4, 1}
  //       |
  //     Dense - copy
  //    {4, 1}

  std::string const input =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});

  std::string const dense_1 =
      g.template AddNode<Dense>("SharedDense", {"Input"}, Dense::AUTODETECT_INPUTS_COUNT, NEURONS);

  std::string const dense_2 = g.template AddNode<Dense>("SharedDense", {dense_1});
  std::string const output  = g.template AddNode<Dense>("SharedDense", {dense_2});

  g.SetInput(input, data);
  g.Compile();

  TensorType const result = g.Evaluate(output);

  math::SizeVector const expected_out_shape{NEURONS, data.shape().back()};
  EXPECT_EQ(result.shape(), expected_out_shape);
}

// (VH): Disabled because shared Dense layers do not work if created with auto-detected inputs.
TYPED_TEST(GraphTest, compute_shapes_two_diamonds_with_shared_ops)
{
  using TensorType = TypeParam;
  using Dense      = fetch::ml::layers::FullyConnected<TensorType>;

  static constexpr math::SizeType NEURONS = 42;

  TensorType data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");

  fetch::ml::Graph<TensorType> g;

  // Note: all 4 Dense nodes share the same single Op.
  //     {4,1}
  //    i_n_p_u_t
  //    |       |
  //  Dense1  Dense1_copy
  //{42, 1}    {42, 1}
  //    |         |
  //  M_U_L_T_I_P_L_Y
  //    |         |
  // Dense2   Dense2_copy
  //{42, 1}    {42, 1}
  //    |         |
  //  M_U_L_T_I_P_L_Y

  std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});

  std::string dense_top_left =
      g.template AddNode<Dense>("SharedDense", {"Input"}, Dense::AUTODETECT_INPUTS_COUNT, NEURONS);

  std::string dense_top_right = g.template AddNode<Dense>("SharedDense", {"Input"});

  std::string multiply1 = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      "Multiply1", {dense_top_left, dense_top_right});

  std::string dense_bottom_left = g.template AddNode<Dense>(
      "SharedDense2", {multiply1}, Dense::AUTODETECT_INPUTS_COUNT, NEURONS);

  std::string dense_bottom_right = g.template AddNode<Dense>("SharedDense2", {"Multiply1"});

  std::string output = g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      "Multiply2", {dense_bottom_left, dense_bottom_right});

  g.SetInput(input, data);
  g.Compile();

  TensorType const result = g.Evaluate(output);

  math::SizeVector const expected_out_shape{NEURONS, data.shape().back()};
  EXPECT_EQ(result.shape(), expected_out_shape);
}

TYPED_TEST(GraphTest, graph_getTrainableNames)
{
  using TensorType = TypeParam;

  // Create graph
  auto g = MakeGraph<TensorType>();

  std::vector<std::string> names = g->GetTrainableNames();

  // Test names
  ASSERT_EQ(names.size(), 6);
  EXPECT_EQ(names.at(0), "FC1/FullyConnected_Bias");
  EXPECT_EQ(names.at(1), "FC1/FullyConnected_Weights");
  EXPECT_EQ(names.at(2), "FC2/FullyConnected_Bias");
  EXPECT_EQ(names.at(3), "FC2/FullyConnected_Weights");
  EXPECT_EQ(names.at(4), "FC3/FullyConnected_Bias");
  EXPECT_EQ(names.at(5), "FC3/FullyConnected_Weights");
}

TYPED_TEST(GraphTest, graph_getNode_as_weight)
{
  using TensorType = TypeParam;

  // Create graph
  auto g = MakeGraph<TensorType>();

  auto node_ptr = g->GetNode("FC3/FullyConnected_Bias");

  ASSERT_TRUE(node_ptr);
  EXPECT_EQ(node_ptr->GetNodeName(), "FullyConnected_Bias");

  auto op_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(node_ptr->GetOp());

  ASSERT_TRUE(op_ptr);

  auto weight = op_ptr->GetWeights();

  // Test weights
  EXPECT_EQ(weight.shape().at(0), 10);
  EXPECT_EQ(weight.shape().at(1), 1);
}

TYPED_TEST(GraphTest, graph_getNode_as_graph)
{
  using TensorType = TypeParam;

  // Create graph
  auto g = MakeGraph<TensorType>();

  auto node_ptr = g->GetNode("FC1");

  ASSERT_TRUE(node_ptr);
  EXPECT_EQ(node_ptr->GetNodeName(), "FC1");

  auto graph_ptr = std::dynamic_pointer_cast<Graph<TensorType>>(node_ptr->GetOp());
  ASSERT_TRUE(graph_ptr);

  std::vector<std::string> names = graph_ptr->GetTrainableNames();

  EXPECT_EQ(names.at(0), "FullyConnected_Bias");
  EXPECT_EQ(names.at(1), "FullyConnected_Weights");
}

TYPED_TEST(GraphTest, graph_invalidName)
{
  using TensorType = TypeParam;

  // Create graph
  fetch::ml::Graph<TensorType> g;

  EXPECT_THROW(g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input/", {}),
               std::runtime_error);
}

TYPED_TEST(GraphTest, graph_getNodeNames)
{
  using TensorType = TypeParam;

  // Create graph
  auto g = MakeGraph<TensorType>();

  std::vector<std::string> names = g->GetNodeNames();

  // Test names
  ASSERT_EQ(names.size(), 26);
  EXPECT_EQ(names.at(0), "FC1");
  EXPECT_EQ(names.at(1), "FC2");
  EXPECT_EQ(names.at(2), "FC3");
  EXPECT_EQ(names.at(3), "Input");
  EXPECT_EQ(names.at(4), "Label");
  EXPECT_EQ(names.at(5), "FC1/FullyConnected_Activation");
  EXPECT_EQ(names.at(6), "FC1/FullyConnected_Add");
  EXPECT_EQ(names.at(7), "FC1/FullyConnected_Bias");
  EXPECT_EQ(names.at(8), "FC1/FullyConnected_Flatten");
  EXPECT_EQ(names.at(9), "FC1/FullyConnected_Input");
  EXPECT_EQ(names.at(10), "FC1/FullyConnected_MatrixMultiply");
  EXPECT_EQ(names.at(11), "FC1/FullyConnected_Weights");
  EXPECT_EQ(names.at(12), "FC2/FullyConnected_Activation");
  EXPECT_EQ(names.at(13), "FC2/FullyConnected_Add");
  EXPECT_EQ(names.at(14), "FC2/FullyConnected_Bias");
  EXPECT_EQ(names.at(15), "FC2/FullyConnected_Flatten");
  EXPECT_EQ(names.at(16), "FC2/FullyConnected_Input");
  EXPECT_EQ(names.at(17), "FC2/FullyConnected_MatrixMultiply");
  EXPECT_EQ(names.at(18), "FC2/FullyConnected_Weights");
  EXPECT_EQ(names.at(19), "FC3/FullyConnected_Activation");
  EXPECT_EQ(names.at(20), "FC3/FullyConnected_Add");
  EXPECT_EQ(names.at(21), "FC3/FullyConnected_Bias");
  EXPECT_EQ(names.at(22), "FC3/FullyConnected_Flatten");
  EXPECT_EQ(names.at(23), "FC3/FullyConnected_Input");
  EXPECT_EQ(names.at(24), "FC3/FullyConnected_MatrixMultiply");
  EXPECT_EQ(names.at(25), "FC3/FullyConnected_Weights");
}

TYPED_TEST(GraphTest, graph_setWeight)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType gt({10, 1});
  gt.Fill(fetch::math::Type<DataType>("1.23"));

  // Create graph
  auto g = MakeGraph<TensorType>();

  // Assign weight
  g->SetWeight("FC3/FullyConnected_Bias", gt);

  // Get weight from graph
  auto node_ptr = g->GetNode("FC3/FullyConnected_Bias");
  ASSERT_TRUE(node_ptr);
  EXPECT_EQ(node_ptr->GetNodeName(), "FullyConnected_Bias");
  auto op_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(node_ptr->GetOp());
  ASSERT_TRUE(op_ptr);
  auto weight = op_ptr->GetWeights();

  // Test size
  EXPECT_EQ(weight.shape().at(0), 10);
  EXPECT_EQ(weight.shape().at(1), 1);

  // Test values
  ASSERT_TRUE(weight.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, graph_getWeightsOrder_1)
{
  // Tests if weight values in GetWeightsReferences are always in alphabetical order

  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  // Create graph
  std::shared_ptr<fetch::ml::Graph<TensorType>> g =
      std::make_shared<fetch::ml::Graph<TensorType>>();

  std::string input = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});

  std::string layer_1 = g->template AddNode<layers::FullyConnected<TensorType>>(
      "B", {input}, 5u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->template AddNode<layers::FullyConnected<TensorType>>(
      "C", {layer_1}, 10u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string output = g->template AddNode<layers::FullyConnected<TensorType>>(
      "A", {layer_2}, 10u, 5u, fetch::ml::details::ActivationType::SOFTMAX);

  TensorType gt_a_bias({5, 1});
  gt_a_bias.Fill(DataType{1});
  TensorType gt_a_weight({10, 5});
  gt_a_weight.Fill(DataType{2});

  TensorType gt_b_bias({10, 1});
  gt_b_bias.Fill(DataType{3});
  TensorType gt_b_weight({5, 10});
  gt_b_weight.Fill(DataType{4});

  TensorType gt_c_bias({10, 1});
  gt_c_bias.Fill(DataType{5});
  TensorType gt_c_weight({10, 10});
  gt_c_weight.Fill(DataType{6});

  g->SetWeight("A/FullyConnected_Bias", gt_a_bias);
  g->SetWeight("A/FullyConnected_Weights", gt_a_weight);

  g->SetWeight("B/FullyConnected_Bias", gt_b_bias);
  g->SetWeight("B/FullyConnected_Weights", gt_b_weight);

  g->SetWeight("C/FullyConnected_Bias", gt_c_bias);
  g->SetWeight("C/FullyConnected_Weights", gt_c_weight);

  auto weights = g->GetWeightsReferences();

  // Test values
  ASSERT_EQ(weights.size(), 6);
  ASSERT_TRUE(weights.at(0).AllClose(gt_a_bias, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights.at(1).AllClose(gt_a_weight, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));

  ASSERT_TRUE(weights.at(2).AllClose(gt_b_bias, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights.at(3).AllClose(gt_b_weight, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));

  ASSERT_TRUE(weights.at(4).AllClose(gt_c_bias, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights.at(5).AllClose(gt_c_weight, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, graph_getWeightsOrder_2)
{
  // Tests if weight values in GetWeightsReferences are always in alphabetical order

  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  // Create graph
  std::shared_ptr<fetch::ml::Graph<TensorType>> g =
      std::make_shared<fetch::ml::Graph<TensorType>>();

  std::string input = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Label", {});

  std::string layer_1 = g->template AddNode<layers::FullyConnected<TensorType>>(
      "C", {input}, 5u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string layer_2 = g->template AddNode<layers::FullyConnected<TensorType>>(
      "A", {layer_1}, 10u, 10u, fetch::ml::details::ActivationType::RELU);
  std::string output = g->template AddNode<layers::FullyConnected<TensorType>>(
      "B", {layer_2}, 10u, 5u, fetch::ml::details::ActivationType::SOFTMAX);

  TensorType gt_a_bias({10, 1});
  gt_a_bias.Fill(DataType{5});
  TensorType gt_a_weight({10, 10});
  gt_a_weight.Fill(DataType{6});

  TensorType gt_b_bias({5, 1});
  gt_b_bias.Fill(DataType{1});
  TensorType gt_b_weight({10, 5});
  gt_b_weight.Fill(DataType{2});

  TensorType gt_c_bias({10, 1});
  gt_c_bias.Fill(DataType{3});
  TensorType gt_c_weight({5, 10});
  gt_c_weight.Fill(DataType{4});

  g->SetWeight("A/FullyConnected_Bias", gt_a_bias);
  g->SetWeight("A/FullyConnected_Weights", gt_a_weight);

  g->SetWeight("B/FullyConnected_Bias", gt_b_bias);
  g->SetWeight("B/FullyConnected_Weights", gt_b_weight);

  g->SetWeight("C/FullyConnected_Bias", gt_c_bias);
  g->SetWeight("C/FullyConnected_Weights", gt_c_weight);

  auto weights = g->GetWeightsReferences();

  // Test values
  ASSERT_EQ(weights.size(), 6);
  ASSERT_TRUE(weights.at(0).AllClose(gt_a_bias, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights.at(1).AllClose(gt_a_weight, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));

  ASSERT_TRUE(weights.at(2).AllClose(gt_b_bias, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights.at(3).AllClose(gt_b_weight, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));

  ASSERT_TRUE(weights.at(4).AllClose(gt_c_bias, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(weights.at(5).AllClose(gt_c_weight, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphTest, graph_charge_input_only)
{
  using TensorType = TypeParam;
  using namespace fetch::ml::ops;

  TensorType data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");

  fetch::ml::Graph<TensorType> g;

  std::string input = g.template AddNode<PlaceHolder<TensorType>>("Input", {});

  g.SetInput(input, data);
  g.Compile();

  OperationsCount const charge          = g.ChargeForward(input);
  OperationsCount const expected_charge = 0;  // Placeholder reading is "free" in charge amount.

  ASSERT_EQ(charge, expected_charge);
}

TYPED_TEST(GraphTest, graph_charge_subtraction)
{
  using TensorType = TypeParam;
  using namespace fetch::ml::ops;
  using namespace fetch::ml::charge_estimation::ops;

  TensorType data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14; 21,22,23,24; 31,32,33,34)");

  fetch::ml::Graph<TensorType> g;

  std::string left_input  = g.template AddNode<PlaceHolder<TensorType>>("LeftInput", {});
  std::string right_input = g.template AddNode<PlaceHolder<TensorType>>("RightInput", {});
  std::string subtract =
      g.template AddNode<Subtract<TensorType>>("Subtract", {left_input, right_input});
  g.SetInput(left_input, data);
  g.SetInput(right_input, data);
  g.Compile();

  OperationsCount const charge       = g.ChargeForward(subtract);
  OperationsCount const batch_charge = charge * data.shape().back();

  std::size_t const     total_elements_in_output = 4 * 4;
  OperationsCount const expected_charge = total_elements_in_output * SUBTRACTION_PER_ELEMENT;

  ASSERT_EQ(batch_charge, expected_charge);
}

TYPED_TEST(GraphTest, graph_charge_matmul)
{
  using namespace fetch::ml::ops;
  using namespace fetch::ml::charge_estimation::ops;
  using TensorType = TypeParam;
  using math::SizeType;

  // MatMul here is intended to multiply 2D weights matrix [2; 4] to a 2D input matrix
  // [4; n], resulting in 2D matrix of size [2; n]; n == batch_size == 6

  TensorType weights_data = TensorType::FromString(R"(01,02,03,04; 11,12,13,14)");
  TensorType input_data   = TensorType::FromString(
      R"(01,02,03,04,05,06; 11,12,13,14,15,16; 21,22,23,24,25,26; 31,32,33,34,35,36)");
  SizeType const weight_width  = weights_data.shape().front();
  SizeType const weight_height = weights_data.shape().back();
  SizeType const input_height  = input_data.shape().front();

  ASSERT_EQ(weight_height, input_height);  // else MatMul is not possible

  SizeType const batch_size = input_data.shape().back();

  fetch::ml::Graph<TensorType> g;

  std::string weights = g.template AddNode<PlaceHolder<TensorType>>("Weights", {});
  std::string input   = g.template AddNode<PlaceHolder<TensorType>>("Input", {});
  std::string matmul =
      g.template AddNode<MatrixMultiply<TensorType>>("MatMul", {"Weights", "Input"});

  g.SetInput(weights, weights_data);
  g.SetInput(input, input_data);
  g.Compile();

  math::SizeVector const out_shape = g.GetNode(matmul)->BatchOutputShape();
  ASSERT_EQ(out_shape.size(), 2);
  ASSERT_EQ(out_shape.front(), 2);

  OperationsCount const charge       = g.ChargeForward(matmul);
  OperationsCount const batch_charge = charge * batch_size;

  SizeType const        matmul_ops      = weight_width * input_height * batch_size;
  OperationsCount const expected_charge = matmul_ops * MULTIPLICATION_PER_ELEMENT;

  ASSERT_EQ(batch_charge, expected_charge);
}

TYPED_TEST(GraphTest, graph_charge_conv2d)
{
  using namespace fetch::ml::ops;
  using namespace fetch::ml::layers;
  using namespace fetch::ml::charge_estimation::ops;
  using TensorType = TypeParam;
  using math::SizeType;

  // TODO: impl. Tensor creation with proper dimensions.
  TensorType input_data = TensorType::FromString(
      R"(01,02,03,04,05,06; 11,12,13,14,15,16; 21,22,23,24,25,26; 31,32,33,34,35,36)");

  SizeType const input_height = input_data.shape().front();

  SizeType const batch_size = input_data.shape().back();

  fetch::ml::Graph<TensorType> g;

  std::string input  = g.template AddNode<PlaceHolder<TensorType>>("Input", {});
  std::string conv2d = g.template AddNode<Convolution2D<TensorType>>("Conv2d", {"Input"});

  g.SetInput(input, input_data);
  g.Compile();

  math::SizeVector const out_shape = g.GetNode(conv2d)->BatchOutputShape();
  ASSERT_EQ(out_shape.size(), 2);
  ASSERT_EQ(out_shape.front(), 2);

  OperationsCount const charge       = g.ChargeForward(conv2d);
  OperationsCount const batch_charge = charge * batch_size;

  SizeType const        matmul_ops      = weight_width * input_height * batch_size;
  OperationsCount const expected_charge = matmul_ops * MULTIPLICATION_PER_ELEMENT;

  ASSERT_EQ(batch_charge, expected_charge);
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
