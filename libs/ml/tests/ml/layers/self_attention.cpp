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

#include "ml/layers/self_attention.hpp"
#include "math/tensor.hpp"
#include "ml/layers/fully_connected.hpp"
#include <gtest/gtest.h>

template <typename T>
class SelfAttentionTest : public ::testing::Test
{
};

// TODO (private 507)
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>>;
TYPED_TEST_CASE(SelfAttentionTest, MyTypes);

TYPED_TEST(SelfAttentionTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::SelfAttention<TypeParam>>("SelfAttention", {"Input"}, 50u,
                                                                  42u, 10u);

  std::shared_ptr<TypeParam> data =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 10}));
  g.SetInput("Input", data);

  std::shared_ptr<TypeParam> prediction = g.Evaluate("SelfAttention");
  //  ASSERT_EQ(prediction->shape().size(), 2);
  //  ASSERT_EQ(prediction->shape()[0], 1);
  //  ASSERT_EQ(prediction->shape()[1], 42);
}

//
// TYPED_TEST(SelfAttentionTest, ops_backward_test)  // Use the class as an Ops
//{
//  fetch::ml::layers::SelfAttention<TypeParam> sa(50, 10);
//  std::shared_ptr<TypeParam>                   inputData =
//      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 10}));
//  std::shared_ptr<TypeParam> output = sa.Forward({inputData});
//  std::shared_ptr<TypeParam> errorSignal =
//      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({1, 10}));
//
//  std::vector<std::shared_ptr<TypeParam>> backpropagatedErrorSignals =
//      sa.Backward({inputData}, errorSignal);
//  ASSERT_EQ(backpropagatedErrorSignals.size(), 1);
//  ASSERT_EQ(backpropagatedErrorSignals[0]->shape().size(), 2);
//  ASSERT_EQ(backpropagatedErrorSignals[0]->shape()[0], 5);
//  ASSERT_EQ(backpropagatedErrorSignals[0]->shape()[1], 10);
//  // No way to test actual values for now as weights are randomly initialised.
//}
//
// TYPED_TEST(SelfAttentionTest, node_forward_test)  // Use the class as a Node
//{
//  std::shared_ptr<TypeParam> data =
//      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 10}));
//  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder
//  =
//      std::make_shared<fetch::ml::Node<TypeParam,
//      fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
//  placeholder->SetData(data);
//
//  fetch::ml::Node<TypeParam, fetch::ml::layers::SelfAttention<TypeParam>> sa(
//      "SelfAttention", 50u, 42u, "SelfAttention");
//  sa.AddInput(placeholder);
//
//  std::shared_ptr<TypeParam> prediction = sa.Evaluate();
//
//  ASSERT_EQ(prediction->shape().size(), 2);
//  ASSERT_EQ(prediction->shape()[0], 1);
//  ASSERT_EQ(prediction->shape()[1], 42);
//}
//
// TYPED_TEST(SelfAttentionTest, node_backward_test)  // Use the class as a Node
//{
//  std::shared_ptr<TypeParam> data =
//      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 10}));
//  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder
//  =
//      std::make_shared<fetch::ml::Node<TypeParam,
//      fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
//  placeholder->SetData(data);
//
//  fetch::ml::Node<TypeParam, fetch::ml::layers::SelfAttention<TypeParam>> sa(
//      "SelfAttention", 50u, 42u, "SelfAttention");
//  sa.AddInput(placeholder);
//  std::shared_ptr<TypeParam> prediction = sa.Evaluate();
//
//  std::shared_ptr<TypeParam> errorSignal =
//      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({1, 42}));
//  auto backpropagatedErrorSignals = sa.BackPropagate(errorSignal);
//
//  ASSERT_EQ(backpropagatedErrorSignals.size(), 1);
//  ASSERT_EQ(backpropagatedErrorSignals[0].second->shape().size(), 2);
//  ASSERT_EQ(backpropagatedErrorSignals[0].second->shape()[0], 5);
//  ASSERT_EQ(backpropagatedErrorSignals[0].second->shape()[1], 10);
//}
//
// TYPED_TEST(SelfAttentionTest, graph_forward_test)  // Use the class as a Node
//{
//  fetch::ml::Graph<TypeParam> g;
//
//  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
//  g.template AddNode<fetch::ml::layers::SelfAttention<TypeParam>>("SelfAttention", {"Input"}, 50u,
//                                                                   42u);
//
//  std::shared_ptr<TypeParam> data =
//      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 10}));
//  g.SetInput("Input", data);
//
//  std::shared_ptr<TypeParam> prediction = g.Evaluate("SelfAttention");
//  ASSERT_EQ(prediction->shape().size(), 2);
//  ASSERT_EQ(prediction->shape()[0], 1);
//  ASSERT_EQ(prediction->shape()[1], 42);
//}
//
// TYPED_TEST(SelfAttentionTest, getStateDict)
//{
//  fetch::ml::layers::SelfAttention<TypeParam> sa(50, 10, "SATest");
//  fetch::ml::ops::StateDict<TypeParam>         sd = sa.StateDict();
//
//  EXPECT_EQ(sd.weights_, nullptr);
//  EXPECT_EQ(sd.dict_.size(), 2);
//
//  ASSERT_TRUE(sd.dict_["SATest_Weights"].weights_ != nullptr);
//  EXPECT_EQ(sd.dict_["SATest_Weights"].weights_->shape(),
//            std::vector<typename TypeParam::SizeType>({50, 10}));
//
//  ASSERT_TRUE(sd.dict_["SATest_Bias"].weights_ != nullptr);
//  EXPECT_EQ(sd.dict_["SATest_Bias"].weights_->shape(),
//            std::vector<typename TypeParam::SizeType>({1, 10}));
//}
