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

#include "ml/ops/fully_connected.hpp"
#include "math/ndarray.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class FullyConnectedTest : public ::testing::Test
{
};

// TODO (private 507)
using MyTypes = ::testing::Types<fetch::math::NDArray<int>, fetch::math::NDArray<float>,
                                 fetch::math::NDArray<double>, fetch::math::Tensor<int>,
                                 fetch::math::Tensor<float>, fetch::math::Tensor<double>>;
TYPED_TEST_CASE(FullyConnectedTest, MyTypes);

TYPED_TEST(FullyConnectedTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::ops::FullyConnected<TypeParam> fc(100u, 10u);
  std::shared_ptr<TypeParam> inputData = std::make_shared<TypeParam>(std::vector<size_t>({10, 10}));
  fc.SetInput("FC_Input", inputData);
  std::shared_ptr<TypeParam> output = fc.Evaluate("FC_MatrixMultiply");

  ASSERT_EQ(output->shape().size(), 2);
  ASSERT_EQ(output->shape()[0], 1);
  ASSERT_EQ(output->shape()[1], 10);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::ops::FullyConnected<TypeParam> fc(50, 10);
  std::shared_ptr<TypeParam> inputData = std::make_shared<TypeParam>(std::vector<size_t>({5, 10}));
  std::shared_ptr<TypeParam> output    = fc.Forward({inputData});

  ASSERT_EQ(output->shape().size(), 2);
  ASSERT_EQ(output->shape()[0], 1);
  ASSERT_EQ(output->shape()[1], 10);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::ops::FullyConnected<TypeParam> fc(50, 10);
  std::shared_ptr<TypeParam> inputData = std::make_shared<TypeParam>(std::vector<size_t>({5, 10}));
  std::shared_ptr<TypeParam> output    = fc.Forward({inputData});
  std::shared_ptr<TypeParam> errorSignal =
      std::make_shared<TypeParam>(std::vector<size_t>({1, 10}));

  std::vector<std::shared_ptr<TypeParam>> backpropagatedErrorSignals =
      fc.Backward({inputData}, errorSignal);
  ASSERT_EQ(backpropagatedErrorSignals.size(), 1);
  ASSERT_EQ(backpropagatedErrorSignals[0]->shape().size(), 2);
  ASSERT_EQ(backpropagatedErrorSignals[0]->shape()[0], 5);
  ASSERT_EQ(backpropagatedErrorSignals[0]->shape()[1], 10);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, node_forward_test)  // Use the class as a Node
{
  std::shared_ptr<TypeParam> data = std::make_shared<TypeParam>(std::vector<size_t>({5, 10}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::ops::FullyConnected<TypeParam>> fc("FullyConnected", 50u,
                                                                           42u, "FullyConnected");
  fc.AddInput(placeholder);

  std::shared_ptr<TypeParam> prediction = fc.Evaluate();

  ASSERT_EQ(prediction->shape().size(), 2);
  ASSERT_EQ(prediction->shape()[0], 1);
  ASSERT_EQ(prediction->shape()[1], 42);
}

TYPED_TEST(FullyConnectedTest, node_backward_test)  // Use the class as a Node
{
  std::shared_ptr<TypeParam> data = std::make_shared<TypeParam>(std::vector<size_t>({5, 10}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::ops::FullyConnected<TypeParam>> fc("FullyConnected", 50u,
                                                                           42u, "FullyConnected");
  fc.AddInput(placeholder);
  std::shared_ptr<TypeParam> prediction = fc.Evaluate();

  std::shared_ptr<TypeParam> errorSignal =
      std::make_shared<TypeParam>(std::vector<size_t>({1, 42}));
  auto backpropagatedErrorSignals = fc.BackPropagate(errorSignal);

  ASSERT_EQ(backpropagatedErrorSignals.size(), 1);
  ASSERT_EQ(backpropagatedErrorSignals[0].second->shape().size(), 2);
  ASSERT_EQ(backpropagatedErrorSignals[0].second->shape()[0], 5);
  ASSERT_EQ(backpropagatedErrorSignals[0].second->shape()[1], 10);
}

TYPED_TEST(FullyConnectedTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::ops::FullyConnected<TypeParam>>("FullyConnected", {"Input"}, 50u,
                                                                42u);

  std::shared_ptr<TypeParam> data = std::make_shared<TypeParam>(std::vector<size_t>({5, 10}));
  g.SetInput("Input", data);

  std::shared_ptr<TypeParam> prediction = g.Evaluate("FullyConnected");
  ASSERT_EQ(prediction->shape().size(), 2);
  ASSERT_EQ(prediction->shape()[0], 1);
  ASSERT_EQ(prediction->shape()[1], 42);
}
