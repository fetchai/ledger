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

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <gtest/gtest.h>

template <typename T>
class PReluTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(PReluTest, MyTypes);

TYPED_TEST(PReluTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::PRelu<TypeParam> fc(100u);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({10, 10, 2}));
  fc.SetInput("PRelu_Input", input_data);
  TypeParam output = fc.Evaluate("PRelu_LeakyReluOp");

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);

  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, ops_forward_test)  // Use the class as an Ops
{
  using VecTensorType = typename fetch::ml::Ops<TypeParam>::VecTensorType;

  fetch::ml::layers::PRelu<TypeParam> pr(50);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  TypeParam output(pr.ComputeOutputShape({input_data}));
  pr.Forward(VecTensorType({input_data}), output);

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::PRelu<TypeParam> fc(50);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  TypeParam output(fc.ComputeOutputShape({input_data}));
  fc.Forward({input_data}, output);
  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({50, 2}));

  std::vector<TypeParam> bp_err = fc.Backward({input_data}, error_signal);
  ASSERT_EQ(bp_err.size(), 1);
  ASSERT_EQ(bp_err[0].shape().size(), 3);
  ASSERT_EQ(bp_err[0].shape()[0], 5);
  ASSERT_EQ(bp_err[0].shape()[1], 10);
  ASSERT_EQ(bp_err[0].shape()[2], 2);

  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::PRelu<TypeParam>> fc("PRelu", 50u, "PRelu");
  fc.AddInput(placeholder);

  TypeParam prediction = fc.Evaluate();

  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 10);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(PReluTest, node_backward_test)  // Use the class as a Node
{
  TypeParam data({5, 10, 2});
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::PRelu<TypeParam>> fc("PRelu", 50u, "PRelu");
  fc.AddInput(placeholder);
  TypeParam prediction = fc.Evaluate();

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  auto      bp_err = fc.BackPropagate(error_signal);

  ASSERT_EQ(bp_err.size(), 1);
  ASSERT_EQ(bp_err[0].second.shape().size(), 3);
  ASSERT_EQ(bp_err[0].second.shape()[0], 5);
  ASSERT_EQ(bp_err[0].second.shape()[1], 10);
  ASSERT_EQ(bp_err[0].second.shape()[2], 2);
}

TYPED_TEST(PReluTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::PRelu<TypeParam>>("PRelu", {"Input"}, 50u);

  TypeParam data({5, 10, 2});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("PRelu");
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 10);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(PReluTest, getStateDict)
{
  fetch::ml::layers::PRelu<TypeParam> fc(50, "PReluTest");
  fetch::ml::StateDict<TypeParam>     sd = fc.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);

  ASSERT_NE(sd.dict_["PReluTest_Alpha"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["PReluTest_Alpha"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({50, 1}));
}
