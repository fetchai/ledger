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

#include "ml/layers/PRelu.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class PReluTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(PReluTest, MyTypes);

TYPED_TEST(PReluTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::PRelu<TypeParam> fc(100u);
  TypeParam inputData(std::vector<typename TypeParam::SizeType>({10, 10}));
  fc.SetInput("PRelu_Input", inputData);
  TypeParam output = fc.Evaluate("PRelu_LeakyReluOp");

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 10);

  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::layers::PRelu<TypeParam> fc(50);
  TypeParam                           inputData(std::vector<typename TypeParam::SizeType>({5, 10}));
  TypeParam                           output = fc.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({inputData}));

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 10);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::PRelu<TypeParam> fc(50);
  TypeParam                           inputData(std::vector<typename TypeParam::SizeType>({5, 10}));
  TypeParam                           output = fc.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({inputData}));
  TypeParam errorSignal(std::vector<typename TypeParam::SizeType>({1, 50}));

  std::vector<TypeParam> backpropagatedErrorSignals = fc.Backward({inputData}, errorSignal);
  ASSERT_EQ(backpropagatedErrorSignals.size(), 1);
  ASSERT_EQ(backpropagatedErrorSignals[0].shape().size(), 2);
  ASSERT_EQ(backpropagatedErrorSignals[0].shape()[0], 1);
  ASSERT_EQ(backpropagatedErrorSignals[0].shape()[1], 50);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::PRelu<TypeParam>> fc("PRelu", 50u, "PRelu");
  fc.AddInput(placeholder);

  TypeParam prediction = fc.Evaluate();

  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 10);
}

TYPED_TEST(PReluTest, node_backward_test)  // Use the class as a Node
{
  TypeParam                                                                           data({5, 10});
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::PRelu<TypeParam>> fc("PRelu", 50u, "PRelu");
  fc.AddInput(placeholder);
  TypeParam prediction = fc.Evaluate();

  TypeParam errorSignal(std::vector<typename TypeParam::SizeType>({1, 50}));
  auto      backpropagatedErrorSignals = fc.BackPropagate(errorSignal);

  ASSERT_EQ(backpropagatedErrorSignals.size(), 1);
  ASSERT_EQ(backpropagatedErrorSignals[0].second.shape().size(), 2);
  ASSERT_EQ(backpropagatedErrorSignals[0].second.shape()[0], 1);
  ASSERT_EQ(backpropagatedErrorSignals[0].second.shape()[1], 50);
}

TYPED_TEST(PReluTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::PRelu<TypeParam>>("PRelu", {"Input"}, 50u);

  TypeParam data({5, 10});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("PRelu");
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 10);
}

TYPED_TEST(PReluTest, getStateDict)
{
  fetch::ml::layers::PRelu<TypeParam> fc(50, "PReluTest");
  fetch::ml::StateDict<TypeParam>     sd = fc.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);

  ASSERT_NE(sd.dict_["PReluTest_Alpha"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["PReluTest_Alpha"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({1, 50}));
}
