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
#include "ml/layers/PRelu.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"
#include "ml/serializers/ml_types.hpp"

#include <memory>
#include <vector>

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
  TypeParam output = fc.Evaluate("PRelu_LeakyReluOp", true);

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 10);
  ASSERT_EQ(output.shape()[2], 2);

  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(PReluTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::layers::PRelu<TypeParam> pr(50);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  TypeParam output(pr.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  pr.Forward({std::make_shared<TypeParam>(input_data)}, output);

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
  TypeParam output(fc.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  fc.Forward({std::make_shared<TypeParam>(input_data)}, output);
  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({50, 2}));

  std::vector<TypeParam> bp_err =
      fc.Backward({std::make_shared<TypeParam>(input_data)}, error_signal);
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
  auto      placeholder_node =
      std::make_shared<fetch::ml::Node<TypeParam>>(fetch::ml::OpType::PLACEHOLDER, "Input");
  auto placeholder_op_ptr =
      std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder_node->GetOp());
  placeholder_op_ptr->SetData(data);

  fetch::math::SizeType in_size = 50u;
  auto prelu_node = fetch::ml::Node<TypeParam>(fetch::ml::OpType::PRELU, "PRelu", [in_size]() {
    return std::make_shared<fetch::ml::layers::PRelu<TypeParam>>(in_size, "PRelu");
  });
  prelu_node.AddInput(placeholder_node);
  auto prediction = prelu_node.Evaluate(true);

  ASSERT_EQ(prediction->shape().size(), 3);
  ASSERT_EQ(prediction->shape()[0], 5);
  ASSERT_EQ(prediction->shape()[1], 10);
  ASSERT_EQ(prediction->shape()[2], 2);
}

TYPED_TEST(PReluTest, node_backward_test)  // Use the class as a Node
{
  TypeParam data({5, 10, 2});
  auto      placeholder_node =
      std::make_shared<fetch::ml::Node<TypeParam>>(fetch::ml::OpType::PLACEHOLDER, "Input");
  auto placeholder_op_ptr =
      std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder_node->GetOp());
  placeholder_op_ptr->SetData(data);

  fetch::math::SizeType in_size = 50u;
  auto prelu_node = fetch::ml::Node<TypeParam>(fetch::ml::OpType::PRELU, "PRelu", [in_size]() {
    return std::make_shared<fetch::ml::layers::PRelu<TypeParam>>(in_size);
  });
  prelu_node.AddInput(placeholder_node);
  TypeParam prediction = *prelu_node.Evaluate(true);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  auto      bp_err = prelu_node.BackPropagateSignal(error_signal);

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

  TypeParam prediction = g.Evaluate("PRelu", true);
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

TYPED_TEST(PReluTest, saveparams_test)
{
  using DataType = typename TypeParam::Type;

  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  fetch::math::SizeType in_size = 50u;
  auto prelu_layer = std::make_shared<fetch::ml::layers::PRelu<TypeParam>>(in_size, "PRelu");

  prelu_layer->SetInput("PRelu_Input", data);

  TypeParam output = prelu_layer->Evaluate("PRelu_LeakyReluOp", true);

  // extract saveparams
  auto sp = prelu_layer->GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<typename fetch::ml::layers::PRelu<TypeParam>::SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<typename fetch::ml::layers::PRelu<TypeParam>::SPType>();
  b >> *dsp2;

  // rebuild
  auto prelu2 = fetch::ml::utilities::BuildLayerPrelu<TypeParam>(*dsp2);

  prelu2->SetInput("PRelu_Input", data);
  TypeParam output2 = prelu2->Evaluate("PRelu_LeakyReluOp", true);

  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}
