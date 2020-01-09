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
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"
#include <memory>
namespace fetch {
namespace ml {
namespace test {
template <typename GraphType, typename TensorType, typename DataType>
std::shared_ptr<GraphType> BuildGraph(bool shared = false, bool time_distributed = false)
{
  using FCType          = fetch::ml::layers::FullyConnected<TensorType>;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;
  using RegType         = fetch::ml::RegularisationType;
  using PlaceHolderType = fetch::ml::ops::PlaceHolder<TensorType>;
  using MSEType         = fetch::ml::ops::MeanSquareErrorLoss<TensorType>;

  auto g = std::make_shared<GraphType>();

  std::string input = g->template AddNode<PlaceHolderType>("Input", {});

  std::string intermediate =
      g->template AddNode<FCType>("FC1", {input}, 10u, 10u, ActivationType::NOTHING, RegType::NONE,
                                  DataType{0}, WeightsInitType::XAVIER_GLOROT, time_distributed);
  std::string layer_name = "FC2";
  if (shared)
  {
    layer_name = "FC1";
  }
  std::string output = g->template AddNode<FCType>(
      layer_name, {intermediate}, 10u, 10u, ActivationType::NOTHING, RegType::NONE, DataType{0},
      WeightsInitType::XAVIER_GLOROT, time_distributed);

  std::string label = g->template AddNode<PlaceHolderType>("Label", {});
  std::string error = g->template AddNode<MSEType>("Error", {output, label});

  g->Compile();
  return g;
}

template <typename T>
class FullyConnectedTest : public ::testing::Test
{
};

TYPED_TEST_CASE(FullyConnectedTest, math::test::TensorFloatingTypes);

TYPED_TEST(FullyConnectedTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(100u, 10u);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({10, 10, 2}));
  fc.SetInput("FullyConnected_Input", input_data);
  TypeParam output = fc.Evaluate("FullyConnected_Add", true);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 2);
  // No way to test actual values for now as weights are randomly initialised.
  // todo: weights and biases can be set with fc.SetInput(name + "_Weights", weights_data) etc.
}

TYPED_TEST(FullyConnectedTest,
           set_input_and_evaluate_test_time_distributed)  // Use the class as a subgraph
{
  using TensorType      = TypeParam;
  using DataType        = typename TensorType::Type;
  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;

  fetch::ml::layers::FullyConnected<TypeParam> fc(10u, 5u, ActivationType::NOTHING, RegType::NONE,
                                                  DataType{0}, WeightsInitType::XAVIER_GLOROT,
                                                  true);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({10, 10, 2}));
  fc.SetInput("TimeDistributed_FullyConnected_Input", input_data);
  TypeParam output = fc.Evaluate("TimeDistributed_FullyConnected_MatrixMultiply", true);

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(50, 10);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  TypeParam output(fc.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  fc.Forward({std::make_shared<TypeParam>(input_data)}, output);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(50, 10);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  TypeParam output(fc.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  fc.Forward({std::make_shared<TypeParam>(input_data)}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({10, 2}));

  std::vector<TypeParam> backprop_error =
      fc.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  auto err_signal = (*(backprop_error.begin()));
  ASSERT_EQ(err_signal.shape().size(), 3);
  ASSERT_EQ(err_signal.shape()[0], 5);
  ASSERT_EQ(err_signal.shape()[1], 10);
  ASSERT_EQ(err_signal.shape()[2], 2);
}

TYPED_TEST(FullyConnectedTest, ops_backward_test_time_distributed)  // Use the class as an Ops
{
  using TensorType      = TypeParam;
  using DataType        = typename TensorType::Type;
  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;

  fetch::ml::layers::FullyConnected<TypeParam> fc(50u, 10u, ActivationType::NOTHING, RegType::NONE,
                                                  DataType{0}, WeightsInitType::XAVIER_GLOROT,
                                                  true);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({50, 10, 2}));

  TypeParam output(fc.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  fc.Forward({std::make_shared<TypeParam>(input_data)}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({10, 10, 2}));

  std::vector<TypeParam> backprop_error =
      fc.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  auto err_signal = (*(backprop_error.begin()));
  ASSERT_EQ(err_signal.shape().size(), 3);
  ASSERT_EQ(err_signal.shape()[0], 50);
  ASSERT_EQ(err_signal.shape()[1], 10);
  ASSERT_EQ(err_signal.shape()[2], 2);
}

TYPED_TEST(FullyConnectedTest, share_weight_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;
  using GraphType  = fetch::ml::Graph<TensorType>;

  // create an auto encoder of two dense layers, both share same weights
  auto g_shared = BuildGraph<GraphType, TensorType, DataType>(true);

  // create an auto encoder of two dense layers, both have different weights
  auto g_not_shared = BuildGraph<GraphType, TensorType, DataType>();

  // check that all weights are equal and create compare list
  auto g_shared_weights_before     = g_shared->GetWeights();
  auto g_not_shared_weights_before = g_not_shared->GetWeights();

  for (std::size_t i = 0; i < 4; i++)
  {
    EXPECT_EQ(g_shared_weights_before[i], g_not_shared_weights_before[i]);
  }

  // start training
  // set data
  TensorType data;
  data.Resize({10, 1});
  for (SizeType i = 0; i < 10; i++)
  {
    data.Set(i, 0, static_cast<DataType>(i));
  }

  g_not_shared->SetInput("Input", data.Copy());
  g_shared->SetInput("Input", data.Copy());

  TensorType pred_not_shared = g_not_shared->Evaluate("FC2");
  TensorType pred_shared     = g_shared->Evaluate("FC1_Copy_1");

  EXPECT_TRUE(pred_shared.AllClose(pred_not_shared, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  // SGD is chosen to be the optimizer to reflect the gradient throw change in weights after 1
  // iteration of training. Run 1 iteration of SGD to train on g shared
  auto                                            lr = DataType{1};
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_shared_optimiser(g_shared, {"Input"}, "Label",
                                                                     "Error", lr);
  g_shared_optimiser.Run({data}, data, 1);
  // Run 1 iteration of SGD to train on g not shared
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_not_shared_optimiser(g_not_shared, {"Input"},
                                                                         "Label", "Error", lr);
  g_not_shared_optimiser.Run({data}, data, 1);

  // check that all weights are equal
  auto g_shared_weights_after     = g_shared->GetWeights();
  auto g_not_shared_weights_after = g_not_shared->GetWeights();

  // check the weights and biases are equal after training for shared weights
  for (std::size_t i = 0; i < 2; i++)
  {
    EXPECT_TRUE(g_shared_weights_after[i] == g_shared_weights_after[i + 2]);
  }

  // check the weights and biases are different after training for not shared weights
  for (std::size_t i = 0; i < 2; i++)
  {
    EXPECT_FALSE(g_not_shared_weights_after[i] == g_not_shared_weights_after[i + 2]);
  }

  // check the gradient of the shared weight matrices are the sum of individual weight matrix
  // gradients in not_shared_graph
  for (std::size_t i = 0; i < 2; i++)
  {
    TensorType shared_gradient = g_shared_weights_after[i] - g_shared_weights_before[i];
    TensorType not_shared_gradient =
        (g_not_shared_weights_after[i] + g_not_shared_weights_after[i + 2]) -
        (g_not_shared_weights_before[i] + g_not_shared_weights_before[i + 2]);

    EXPECT_TRUE(shared_gradient.AllClose(
        not_shared_gradient, DataType{10} * fetch::math::function_tolerance<DataType>()));
  }
}

TYPED_TEST(FullyConnectedTest, share_weight_backward_test_time_distributed)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;
  using GraphType  = fetch::ml::Graph<TensorType>;
  using FCType     = fetch::ml::layers::FullyConnected<TensorType>;

  std::string descriptor = FCType::DESCRIPTOR;

  // create an auto encoder of two dense layers, both share same weights
  auto g_shared = BuildGraph<GraphType, TensorType, DataType>(true, true);

  // create an auto encoder of two dense layers, both have different weights
  auto g_not_shared = BuildGraph<GraphType, TensorType, DataType>(false, true);

  // check that all weights are equal and create compare list
  auto g_shared_weights_before     = g_shared->GetWeights();
  auto g_not_shared_weights_before = g_not_shared->GetWeights();

  for (std::size_t i = 0; i < 4; i++)
  {
    ASSERT_EQ(g_shared_weights_before[i], g_not_shared_weights_before[i]);
  }

  // start training
  // set data
  TensorType data;
  data.Resize({20, 1});
  for (SizeType i = 0; i < 20; i++)
  {
    data.Set(i, 0, static_cast<DataType>(i));
  }
  data.Reshape({10, 2, 1});

  g_not_shared->SetInput("Input", data.Copy());
  g_shared->SetInput("Input", data.Copy());

  TensorType pred_not_shared = g_not_shared->Evaluate("FC2");
  TensorType pred_shared     = g_shared->Evaluate("FC1_Copy_1");

  EXPECT_TRUE(pred_shared.AllClose(pred_not_shared, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  //   SGD is chosen to be the optimizer to reflect the gradient throw change in weights after 1
  //   iteration of training. Run 1 iteration of SGD to train on g shared
  auto                                            lr = fetch::math::Type<DataType>("0.01");
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_shared_optimiser(g_shared, {"Input"}, "Label",
                                                                     "Error", lr);
  DataType shared_loss = g_shared_optimiser.Run({data}, data, 1);

  //   Run 1 iteration of SGD to train on g not shared
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_not_shared_optimiser(g_not_shared, {"Input"},
                                                                         "Label", "Error", lr);
  DataType not_shared_loss = g_not_shared_optimiser.Run({data}, data, 1);

  EXPECT_EQ(shared_loss, not_shared_loss);

  // check that all weights are equal
  auto g_shared_weights_after     = g_shared->GetWeights();
  auto g_not_shared_weights_after = g_not_shared->GetWeights();

  // check the all weights are initialized to be the same
  for (std::size_t i = 0; i < 2; i++)
  {
    EXPECT_TRUE(g_shared_weights_before[i] == g_shared_weights_before[i + 2]);
    EXPECT_TRUE(g_not_shared_weights_before[i] == g_not_shared_weights_before[i + 2]);
  }

  // check the all weights are initialized to be the same
  for (std::size_t i = 0; i < 2; i++)
  {
    EXPECT_TRUE(g_shared_weights_before[i] == g_shared_weights_before[i + 2]);
    EXPECT_TRUE(g_not_shared_weights_before[i] == g_not_shared_weights_before[i + 2]);
  }

  // check the weights are equal after training for shared weights
  for (std::size_t i = 0; i < 2; i++)
  {
    EXPECT_TRUE(g_shared_weights_after[i] == g_shared_weights_after[i + 2]);
  }

  // check the weights are different after training for not shared weights
  for (std::size_t i = 0; i < 2; i++)
  {
    ASSERT_FALSE(g_not_shared_weights_after[i] == g_not_shared_weights_after[i + 2]);
  }

  // check the gradient of the shared weight matrices are the sum of individual weight matrice
  // gradients in not_shared_graph
  for (std::size_t i = 0; i < 2; i++)
  {
    TensorType shared_gradient = g_shared_weights_after[i] - g_shared_weights_before[i];
    TensorType not_shared_gradient =
        g_not_shared_weights_after[i] + g_not_shared_weights_after[i + 2] -
        g_not_shared_weights_before[i] - g_not_shared_weights_before[i + 2];

    EXPECT_TRUE(shared_gradient.AllClose(
        not_shared_gradient, DataType{10} * fetch::math::function_tolerance<DataType>()));
  }
}

TYPED_TEST(FullyConnectedTest, share_weight_cache_clearining_check)
{
  using TensorType      = TypeParam;
  using DataType        = typename TensorType::Type;
  using SizeType        = fetch::math::SizeType;
  using GraphType       = fetch::ml::Graph<TensorType>;
  using FCType          = fetch::ml::layers::FullyConnected<TensorType>;
  using PlaceHolderType = fetch::ml::ops::PlaceHolder<TensorType>;
  using MSEType         = fetch::ml::ops::MeanSquareErrorLoss<TensorType>;

  // graph with two shared fully connected layers. We only backprop through one and check the other
  // updates Input -> FC_1        -> LossOp
  //       -> FC_1_Shared

  auto        g          = std::make_shared<GraphType>();
  std::string input      = g->template AddNode<PlaceHolderType>("Input", {});
  std::string fc1        = g->template AddNode<FCType>("FC1", {input}, 10u, 10u);
  std::string fc1_shared = g->template AddNode<FCType>("FC1", {input}, 10u, 10u);
  std::string label      = g->template AddNode<PlaceHolderType>("Label", {});
  std::string error      = g->template AddNode<MSEType>("Error", {fc1, label});
  g->Compile();

  // check that all weights are equal and create compare list
  auto g_shared_weights_before = g->GetWeights();

  // check that weight initialisation is shared for biases and weights
  EXPECT_TRUE(g_shared_weights_before.size() == 4);
  EXPECT_TRUE(g_shared_weights_before.at(0) == g_shared_weights_before.at(2));
  EXPECT_TRUE(g_shared_weights_before.at(1) == g_shared_weights_before.at(3));

  // start training
  // set data
  TensorType data;
  data.Resize({10, 1});
  for (SizeType i = 0; i < 10; i++)
  {
    data.Set(i, 0, static_cast<DataType>(i));
  }

  g->SetInput("Input", data.Copy());
  TensorType fc1_pred        = g->Evaluate(fc1);
  TensorType fc1_pred_shared = g->Evaluate(fc1_shared);

  EXPECT_TRUE(fc1_pred.AllClose(fc1_pred_shared, DataType{0}, DataType{0}));

  auto                                            lr = DataType{1};
  fetch::ml::optimisers::SGDOptimiser<TensorType> opt(g, {"Input"}, "Label", "Error", lr);
  opt.Run({data}, data, 1);

  TensorType fc1_pred_after        = g->Evaluate(fc1);
  TensorType fc1_pred_shared_after = g->Evaluate(fc1_shared);

  auto g_shared_weights_after = g->GetWeights();

  // check weights and biases are the same
  EXPECT_TRUE(g_shared_weights_after.at(0).AllClose(g_shared_weights_after.at(2), DataType{0},
                                                    DataType{0}));
  EXPECT_TRUE(g_shared_weights_after.at(1).AllClose(g_shared_weights_after.at(3), DataType{0},
                                                    DataType{0}));

  // check layer predictions are the same
  EXPECT_FALSE(fc1_pred.AllClose(fc1_pred_after, DataType{0}, DataType{0}));
  EXPECT_FALSE(fc1_pred_shared.AllClose(fc1_pred_shared_after, DataType{0}, DataType{0}));

  // check both shared layers still give the same prediction
  EXPECT_TRUE(fc1_pred_after.AllClose(fc1_pred_shared_after, DataType{0}, DataType{0}));
}

TYPED_TEST(FullyConnectedTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  std::shared_ptr<fetch::ml::Node<TypeParam>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam>>(
          fetch::ml::OpType::OP_PLACEHOLDER, "Input",
          []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder->GetOp())
      ->SetData(data);

  fetch::math::SizeType      in_size  = 50u;
  fetch::math::SizeType      out_size = 42u;
  fetch::ml::Node<TypeParam> fc(
      fetch::ml::OpType::LAYER_FULLY_CONNECTED, "FullyConnected", [in_size, out_size]() {
        return std::make_shared<fetch::ml::layers::FullyConnected<TypeParam>>(in_size, out_size);
      });
  fc.AddInput(placeholder);

  TypeParam prediction = *fc.Evaluate(true);

  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 2);
}

TYPED_TEST(FullyConnectedTest, node_backward_test)  // Use the class as a Node
{
  TypeParam                                   data({5, 10, 2});
  std::shared_ptr<fetch::ml::Node<TypeParam>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam>>(
          fetch::ml::OpType::OP_PLACEHOLDER, "Input",
          []() { return std::make_shared<fetch::ml::ops::PlaceHolder<TypeParam>>(); });
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder->GetOp())
      ->SetData(data);

  fetch::math::SizeType      in_size  = 50u;
  fetch::math::SizeType      out_size = 42u;
  fetch::ml::Node<TypeParam> fc(
      fetch::ml::OpType::LAYER_FULLY_CONNECTED, "FullyConnected", [in_size, out_size]() {
        return std::make_shared<fetch::ml::layers::FullyConnected<TypeParam>>(in_size, out_size);
      });
  fc.AddInput(placeholder);
  TypeParam prediction = *fc.Evaluate(true);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({42, 2}));
  auto      backprop_error = fc.BackPropagate(error_signal);

  ASSERT_EQ(backprop_error.size(), 1);
  auto err_sig = (*(backprop_error.begin())).second.at(0);
  ASSERT_EQ(err_sig.shape().size(), 3);
  ASSERT_EQ(err_sig.shape()[0], 5);
  ASSERT_EQ(err_sig.shape()[1], 10);
  ASSERT_EQ(err_sig.shape()[2], 2);
}

TYPED_TEST(FullyConnectedTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("FullyConnected", {"Input"}, 50u,
                                                                   42u);

  TypeParam data({5, 10, 2});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("FullyConnected", true);
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 2);
}

TYPED_TEST(FullyConnectedTest, getStateDict)
{
  using DataType = typename TypeParam::Type;
  using RegType  = fetch::ml::RegularisationType;

  fetch::ml::layers::FullyConnected<TypeParam> fc(
      50, 10, fetch::ml::details::ActivationType::NOTHING, RegType::NONE, DataType{0});
  fetch::ml::StateDict<TypeParam> sd = fc.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 2);

  ASSERT_NE(sd.dict_["FullyConnected_Weights"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["FullyConnected_Weights"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 50}));

  ASSERT_NE(sd.dict_["FullyConnected_Bias"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["FullyConnected_Bias"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 1}));
}

TYPED_TEST(FullyConnectedTest, getStateDict_time_distributed)
{
  using DataType        = typename TypeParam::Type;
  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;

  fetch::ml::layers::FullyConnected<TypeParam> fc(
      50, 10, fetch::ml::details::ActivationType::NOTHING, RegType::NONE, DataType{0},
      WeightsInitType::XAVIER_GLOROT, true);
  fetch::ml::StateDict<TypeParam> sd = fc.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 2);

  ASSERT_NE(sd.dict_["TimeDistributed_FullyConnected_Weights"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["TimeDistributed_FullyConnected_Weights"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 50}));

  ASSERT_NE(sd.dict_["TimeDistributed_FullyConnected_Bias"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["TimeDistributed_FullyConnected_Bias"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 1, 1}));
}

TYPED_TEST(FullyConnectedTest, training_should_change_output)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = fetch::math::SizeType;
  using LayerType = typename fetch::ml::layers::FullyConnected<TypeParam>;

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

  TypeParam prediction3 = layer.Evaluate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(FullyConnectedTest, saveparams_test)
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

}  // namespace test
}  // namespace ml
}  // namespace fetch
