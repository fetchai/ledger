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
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class FullyConnectedTest : public ::testing::Test
{
};

template <typename T>
class FullyConnectedTestNoInt : public ::testing::Test
{
};

using NoIntTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                    fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                    fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;

TYPED_TEST_CASE(FullyConnectedTest, NoIntTypes);

TYPED_TEST(FullyConnectedTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(100u, 10u);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({10, 10, 2}));
  fc.SetInput("FullyConnected_Input", input_data);
  TypeParam output = fc.Evaluate("FullyConnected_Add", true);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 2);
  // No way to test actual values for now as weights are randomly initialised.  // todo: true?
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
                                                  static_cast<DataType>(0),
                                                  WeightsInitType::XAVIER_GLOROT, true);
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
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 5);
  ASSERT_EQ(backprop_error[0].shape()[1], 10);
  ASSERT_EQ(backprop_error[0].shape()[2], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_backward_test_time_distributed)  // Use the class as an Ops
{
  using TensorType      = TypeParam;
  using DataType        = typename TensorType::Type;
  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;

  fetch::ml::layers::FullyConnected<TypeParam> fc(50u, 10u, ActivationType::NOTHING, RegType::NONE,
                                                  static_cast<DataType>(0),
                                                  WeightsInitType::XAVIER_GLOROT, true);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({50, 10, 2}));

  TypeParam output(fc.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  fc.Forward({std::make_shared<TypeParam>(input_data)}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({10, 10, 2}));

  std::vector<TypeParam> backprop_error =
      fc.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 50);
  ASSERT_EQ(backprop_error[0].shape()[1], 10);
  ASSERT_EQ(backprop_error[0].shape()[2], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, share_weight_backward_test)
{
  using TensorType      = TypeParam;
  using DataType        = typename TensorType::Type;
  using SizeType        = typename TensorType::SizeType;
  using GraphType       = typename fetch::ml::Graph<TensorType>;
  using FCType          = typename fetch::ml::layers::FullyConnected<TensorType>;
  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;

  std::string descriptor = FCType::DESCRIPTOR;

  // create an auto encoder of two dense layers, both share same weights
  auto g_shared = std::make_shared<GraphType>();

  std::string g_shared_input =
      g_shared->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string g_shared_intermediate = g_shared->template AddNode<FCType>(
      "FC1", {g_shared_input}, 10u, 10u, ActivationType::NOTHING, RegType::NONE,
      static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT);
  std::string g_shared_output = g_shared->template AddNode<FCType>(
      "FC1", {g_shared_intermediate}, 10u, 10u, ActivationType::NOTHING, RegType::NONE,
      static_cast<DataType>(0));
  std::string g_shared_label =
      g_shared->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Label", {});
  std::string g_shared_error =
      g_shared->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
          "Error", {g_shared_output, g_shared_label});

  // create an auto encoder of two dense layers, both have different weights
  auto g_not_shared = std::make_shared<GraphType>();

  std::string g_not_shared_input =
      g_not_shared->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string g_not_shared_intermediate = g_not_shared->template AddNode<FCType>(
      "FC4", {g_not_shared_input}, 10u, 10u, ActivationType::NOTHING, RegType::NONE,
      static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT);
  std::string g_not_shared_output = g_not_shared->template AddNode<FCType>(
      "FC5", {g_not_shared_intermediate}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      RegType::NONE, static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT);
  std::string g_not_shared_label =
      g_not_shared->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Label", {});
  std::string g_not_shared_error =
      g_not_shared->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
          "Error", {g_not_shared_output, g_not_shared_label});

  // check that all weights are equal and create compare list
  auto                    g_shared_statedict_before = g_shared->StateDict();
  std::vector<TensorType> g_shared_weights_before;

  // check the naming is as expected
  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_FullyConnected_Weights"].weights_->Copy());
  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_FullyConnected_Bias"].weights_->Copy());

  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_FullyConnected_Weights"].weights_->Copy());
  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_FullyConnected_Bias"].weights_->Copy());

  auto                    g_not_shared_statedict_before = g_not_shared->StateDict();
  std::vector<TensorType> g_not_shared_weights_before;

  // check the naming is as expected
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC4_FullyConnected_Weights"].weights_->Copy());
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC4_FullyConnected_Bias"].weights_->Copy());
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC5_FullyConnected_Weights"].weights_->Copy());
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC5_FullyConnected_Bias"].weights_->Copy());

  for (size_t i = 0; i < 4; i++)
  {
    ASSERT_EQ(g_shared_weights_before[i], g_not_shared_weights_before[i]);
  }

  // start training
  // set data
  TensorType data;
  data.Resize({10, 1});
  for (SizeType i = 0; i < 10; i++)
  {
    data.Set(i, 0, static_cast<DataType>(i));
  }

  // SGD is chosen to be the optimizer to reflect the gradient throw change in weights after 1
  // iteration of training. Run 1 iteration of SGD to train on g shared
  auto                                            lr = static_cast<DataType>(1);
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_shared_optimiser(
      g_shared, {g_shared_input}, g_shared_label, g_shared_error, lr);
  g_shared_optimiser.Run({data}, data, 1);
  // Run 1 iteration of SGD to train on g not shared
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_not_shared_optimiser(
      g_not_shared, {g_not_shared_input}, g_not_shared_label, g_not_shared_error, lr);
  g_not_shared_optimiser.Run({data}, data, 1);

  // check that all weights are equal
  auto                    g_shared_statedict_after = g_shared->StateDict();
  std::vector<TensorType> g_shared_weights_after;
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_FullyConnected_Weights"].weights_->Copy());
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_FullyConnected_Bias"].weights_->Copy());
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_FullyConnected_Weights"].weights_->Copy());
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_FullyConnected_Bias"].weights_->Copy());

  auto                    g_not_shared_statedict_after = g_not_shared->StateDict();
  std::vector<TensorType> g_not_shared_weights_after;
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC4_FullyConnected_Weights"].weights_->Copy());
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC4_FullyConnected_Bias"].weights_->Copy());
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC5_FullyConnected_Weights"].weights_->Copy());
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC5_FullyConnected_Bias"].weights_->Copy());

  // check the all weights are initialized to be the same
  for (size_t i = 0; i < 2; i++)
  {
    ASSERT_TRUE(g_shared_weights_before[i] == g_shared_weights_before[i + 2]);
    ASSERT_TRUE(g_not_shared_weights_before[i] == g_not_shared_weights_before[i + 2]);
  }

  // check the weights are equal after training for shared weights
  for (size_t i = 0; i < 2; i++)
  {
    ASSERT_TRUE(g_shared_weights_after[i] == g_shared_weights_after[i + 2]);
  }

  // check the weights are different after training for not shared weights
  for (size_t i = 0; i < 2; i++)
  {
    ASSERT_FALSE(g_not_shared_weights_after[i] == g_not_shared_weights_after[i + 2]);
  }

  // check the gradient of the shared weight matrices are the sum of individual weight matrice
  // gradients in not_shared_graph
  for (size_t i = 0; i < 2; i++)
  {
    TensorType shared_gradient = g_shared_weights_after[i] - g_shared_weights_before[i];
    TensorType not_shared_gradient =
        g_not_shared_weights_after[i] + g_not_shared_weights_after[i + 2] -
        g_not_shared_weights_before[i] - g_not_shared_weights_before[i + 2];

    ASSERT_TRUE(shared_gradient.AllClose(
        not_shared_gradient,
        static_cast<DataType>(100) * fetch::math::function_tolerance<DataType>()));
  }
}

TYPED_TEST(FullyConnectedTest, share_weight_backward_test_time_distributed)
{
  using TensorType      = TypeParam;
  using DataType        = typename TensorType::Type;
  using SizeType        = typename TensorType::SizeType;
  using GraphType       = typename fetch::ml::Graph<TensorType>;
  using FCType          = typename fetch::ml::layers::FullyConnected<TensorType>;
  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;

  std::string descriptor = FCType::DESCRIPTOR;

  // create an auto encoder of two dense layers, both share same weights
  auto g_shared = std::make_shared<GraphType>();

  std::string g_shared_input =
      g_shared->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string g_shared_intermediate = g_shared->template AddNode<FCType>(
      "FC1", {g_shared_input}, 10u, 10u, ActivationType::NOTHING, RegType::NONE,
      static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
  std::string g_shared_output = g_shared->template AddNode<FCType>(
      "FC1", {g_shared_intermediate}, 10u, 10u, ActivationType::NOTHING, RegType::NONE,
      static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
  std::string g_shared_label =
      g_shared->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Label", {});
  std::string g_shared_error =
      g_shared->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
          "Error", {g_shared_output, g_shared_label});

  // create an auto encoder of two dense layers, both have different weights
  auto g_not_shared = std::make_shared<GraphType>();

  std::string g_not_shared_input =
      g_not_shared->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string g_not_shared_intermediate = g_not_shared->template AddNode<FCType>(
      "FC4", {g_not_shared_input}, 10u, 10u, ActivationType::NOTHING, RegType::NONE,
      static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
  std::string g_not_shared_output = g_not_shared->template AddNode<FCType>(
      "FC5", {g_not_shared_intermediate}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING,
      RegType::NONE, static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
  std::string g_not_shared_label =
      g_not_shared->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Label", {});
  std::string g_not_shared_error =
      g_not_shared->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
          "Error", {g_not_shared_output, g_not_shared_label});

  // check that all weights are equal and create compare list
  auto                    g_shared_statedict_before = g_shared->StateDict();
  std::vector<TensorType> g_shared_weights_before;

  // check the naming is as expected
  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_TimeDistributed_FullyConnected_Bias"].weights_->Copy());
  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_Copy_1_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_shared_weights_before.emplace_back(
      g_shared_statedict_before.dict_["FC1_Copy_1_TimeDistributed_FullyConnected_Bias"]
          .weights_->Copy());

  auto                    g_not_shared_statedict_before = g_not_shared->StateDict();
  std::vector<TensorType> g_not_shared_weights_before;

  // check the naming is as expected
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC4_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC4_TimeDistributed_FullyConnected_Bias"]
          .weights_->Copy());
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC5_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_not_shared_weights_before.emplace_back(
      g_not_shared_statedict_before.dict_["FC5_TimeDistributed_FullyConnected_Bias"]
          .weights_->Copy());

  for (size_t i = 0; i < 4; i++)
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

  // SGD is chosen to be the optimizer to reflect the gradient throw change in weights after 1
  // iteration of training. Run 1 iteration of SGD to train on g shared
  auto                                            lr = static_cast<DataType>(0.01);
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_shared_optimiser(
      g_shared, {g_shared_input}, g_shared_label, g_shared_error, lr);
  g_shared_optimiser.Run({data}, data, 1);
  // Run 1 iteration of SGD to train on g not shared
  fetch::ml::optimisers::SGDOptimiser<TensorType> g_not_shared_optimiser(
      g_not_shared, {g_not_shared_input}, g_not_shared_label, g_not_shared_error, lr);
  g_not_shared_optimiser.Run({data}, data, 1);

  // check that all weights are equal
  auto                    g_shared_statedict_after = g_shared->StateDict();
  std::vector<TensorType> g_shared_weights_after;
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_TimeDistributed_FullyConnected_Bias"].weights_->Copy());
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_Copy_1_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_shared_weights_after.emplace_back(
      g_shared_statedict_after.dict_["FC1_Copy_1_TimeDistributed_FullyConnected_Bias"]
          .weights_->Copy());

  auto                    g_not_shared_statedict_after = g_not_shared->StateDict();
  std::vector<TensorType> g_not_shared_weights_after;
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC4_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC4_TimeDistributed_FullyConnected_Bias"]
          .weights_->Copy());
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC5_TimeDistributed_FullyConnected_Weights"]
          .weights_->Copy());
  g_not_shared_weights_after.emplace_back(
      g_not_shared_statedict_after.dict_["FC5_TimeDistributed_FullyConnected_Bias"]
          .weights_->Copy());

  // check the all weights are initialized to be the same
  for (size_t i = 0; i < 2; i++)
  {
    ASSERT_TRUE(g_shared_weights_before[i] == g_shared_weights_before[i + 2]);
    ASSERT_TRUE(g_not_shared_weights_before[i] == g_not_shared_weights_before[i + 2]);
  }

  // check the weights are equal after training for shared weights
  for (size_t i = 0; i < 2; i++)
  {
    ASSERT_TRUE(g_shared_weights_after[i] == g_shared_weights_after[i + 2]);
  }

  // check the weights are different after training for not shared weights
  for (size_t i = 0; i < 2; i++)
  {
    ASSERT_FALSE(g_not_shared_weights_after[i] == g_not_shared_weights_after[i + 2]);
  }

  // check the gradient of the shared weight matrices are the sum of individual weight matrice
  // gradients in not_shared_graph
  for (size_t i = 0; i < 2; i++)
  {
    TensorType shared_gradient = g_shared_weights_after[i] - g_shared_weights_before[i];
    TensorType not_shared_gradient =
        g_not_shared_weights_after[i] + g_not_shared_weights_after[i + 2] -
        g_not_shared_weights_before[i] - g_not_shared_weights_before[i + 2];
    ASSERT_TRUE(shared_gradient.AllClose(
        not_shared_gradient,
        static_cast<DataType>(100) * fetch::math::function_tolerance<DataType>()));
  }
}

TYPED_TEST(FullyConnectedTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  std::shared_ptr<fetch::ml::Node<TypeParam>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam>>(fetch::ml::OpType::OP_PLACEHOLDER, "Input");
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
      std::make_shared<fetch::ml::Node<TypeParam>>(fetch::ml::OpType::OP_PLACEHOLDER, "Input");
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
  auto      backprop_error = fc.BackPropagateSignal(error_signal);

  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].second.shape().size(), 3);
  ASSERT_EQ(backprop_error[0].second.shape()[0], 5);
  ASSERT_EQ(backprop_error[0].second.shape()[1], 10);
  ASSERT_EQ(backprop_error[0].second.shape()[2], 2);
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

TYPED_TEST(FullyConnectedTest, saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;
  using LayerType = typename fetch::ml::layers::FullyConnected<TypeParam>;
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

  ASSERT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TypeParam loss = layer.Evaluate(error_output);
  layer.BackPropagateError(error_output);
  layer.Step(DataType{0.1f});

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagateError(error_output);
  layer2.Step(DataType{0.1f});

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
