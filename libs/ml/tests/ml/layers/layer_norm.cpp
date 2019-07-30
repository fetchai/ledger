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

#include "math/fundamental_operators.hpp"
#include "math/tensor.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class LayerNormTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(LayerNormTest, MyTypes);

TYPED_TEST(LayerNormTest, set_input_and_evaluate_test_2D)  // Use the class as a subgraph
{
  fetch::ml::layers::LayerNorm<TypeParam> ln({100u, 10u});
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({100, 10, 2}));
  ln.SetInput("LN_Input", input_data);
  TypeParam output = ln.Evaluate("LayerNorm_Beta_Addition", true);

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 100);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);
}

TYPED_TEST(LayerNormTest, forward_test_1D)  // Use the class as a subgraph
{
  fetch::ml::layers::LayerNorm<TypeParam> ln({100u});
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({100, 2}));
  TypeParam output(ln.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  ln.Forward({std::make_shared<TypeParam>(input_data)}, output);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 100);
  ASSERT_EQ(output.shape()[1], 2);
}

TYPED_TEST(LayerNormTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::LayerNorm<TypeParam> ln({50, 10});
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({50, 10, 2}));

  TypeParam output(ln.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  ln.Forward({std::make_shared<TypeParam>(input_data)}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({50, 10, 2}));

  std::vector<TypeParam> backprop_error =
      ln.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 50);
  ASSERT_EQ(backprop_error[0].shape()[1], 10);
  ASSERT_EQ(backprop_error[0].shape()[2], 2);
}

TYPED_TEST(LayerNormTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::LayerNorm<TypeParam>> ln(
      "LayerNorm", std::vector<typename TypeParam::SizeType>({5, 10}));
  ln.AddInput(placeholder);

  TypeParam prediction = *ln.Evaluate(true);

  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 10);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(LayerNormTest, node_backward_test)  // Use the class as a Node
{
  TypeParam data({5, 10, 2});
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::LayerNorm<TypeParam>> ln(
      "LayerNorm", std::vector<typename TypeParam::SizeType>({5, 10}));
  ln.AddInput(placeholder);

  TypeParam prediction = *ln.Evaluate(true);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  auto      backprop_error = ln.BackPropagateSignal(error_signal);

  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].second.shape().size(), 3);
  ASSERT_EQ(backprop_error[0].second.shape()[0], 5);
  ASSERT_EQ(backprop_error[0].second.shape()[1], 10);
  ASSERT_EQ(backprop_error[0].second.shape()[2], 2);
}

TYPED_TEST(LayerNormTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::LayerNorm<TypeParam>>(
      "LayerNorm", {"Input"}, std::vector<typename TypeParam::SizeType>({5, 10}));

  TypeParam data({5, 10, 2});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("FullyConnected", true);
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 10);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(LayerNormTest, getStateDict)
{
  fetch::ml::layers::LayerNorm<TypeParam> ln({50, 10});
  fetch::ml::StateDict<TypeParam>         sd = ln.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 2);

  ASSERT_NE(sd.dict_["LayerNorm_Gamma"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["LayerNorm_Gamma"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({50, 1, 1}));

  ASSERT_NE(sd.dict_["LayerNorm_Beta"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["LayerNorm_Beta"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({50, 1, 1}));
}
