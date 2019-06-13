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

#include "ml/layers/fully_connected.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <gtest/gtest.h>

template <typename T>
class FullyConnectedTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(FullyConnectedTest, MyTypes);

TYPED_TEST(FullyConnectedTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(100u, 10u);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({10, 10}));
  fc.SetInput("FC_Input", input_data);
  TypeParam output = fc.Evaluate("FC_MatrixMultiply");

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 1);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(50, 10);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10}));

  TypeParam output(fc.ComputeOutputShape({input_data}));
  fc.Forward({input_data}, output);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 1);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(50, 10);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10}));

  TypeParam output(fc.ComputeOutputShape({input_data}));
  fc.Forward({input_data}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({10, 1}));

  std::vector<TypeParam> backprop_error = fc.Backward({input_data}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 2);
  ASSERT_EQ(backprop_error[0].shape()[0], 5);
  ASSERT_EQ(backprop_error[0].shape()[1], 10);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::FullyConnected<TypeParam>> fc("FullyConnected", 50u,
                                                                              42u);
  fc.AddInput(placeholder);

  TypeParam prediction = fc.Evaluate();

  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 1);
}

TYPED_TEST(FullyConnectedTest, node_backward_test)  // Use the class as a Node
{
  TypeParam                                                                           data({5, 10});
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::FullyConnected<TypeParam>> fc("FullyConnected", 50u,
                                                                              42u);
  fc.AddInput(placeholder);
  TypeParam prediction = fc.Evaluate();

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({42, 1}));
  auto      backprop_error = fc.BackPropagate(error_signal);

  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].second.shape().size(), 2);
  ASSERT_EQ(backprop_error[0].second.shape()[0], 5);
  ASSERT_EQ(backprop_error[0].second.shape()[1], 10);
}

TYPED_TEST(FullyConnectedTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("FullyConnected", {"Input"}, 50u,
                                                                   42u);

  TypeParam data({5, 10});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("FullyConnected");
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 1);
}

TYPED_TEST(FullyConnectedTest, getStateDict)
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(
      50, 10, fetch::ml::details::ActivationType::NOTHING, "FCTest");
  fetch::ml::StateDict<TypeParam> sd = fc.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 2);

  ASSERT_NE(sd.dict_["FCTest_Weights"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["FCTest_Weights"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 50}));

  ASSERT_NE(sd.dict_["FCTest_Bias"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["FCTest_Bias"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 1}));
}
