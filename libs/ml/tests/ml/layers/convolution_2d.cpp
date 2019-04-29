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

#include "ml/layers/convolution_2d.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class Convolution2DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(Convolution2DTest, MyTypes);

TYPED_TEST(Convolution2DTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::Convolution2D<TypeParam> conv(5u, 3u, 3u, 1u);
  TypeParam inputData(std::vector<typename TypeParam::SizeType>({3, 3, 3}));
  conv.SetInput("Conv2D_Input", inputData);
  TypeParam output = conv.Evaluate("Conv2D_Conv2D");

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 1);
  ASSERT_EQ(output.shape()[2], 1);

  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(Convolution2DTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::layers::Convolution2D<TypeParam> conv(5u, 3u, 3u, 2u);
  TypeParam inputData(std::vector<typename TypeParam::SizeType>({3, 5, 5}));
  TypeParam output = conv.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({inputData}));

  ASSERT_EQ(output.shape().size(), 3);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 2);
  ASSERT_EQ(output.shape()[2], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(Convolution2DTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::Convolution2D<TypeParam> conv(5u, 3u, 3u, 2u);
  TypeParam inputData(std::vector<typename TypeParam::SizeType>({3, 5, 5}));
  TypeParam output = conv.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({inputData}));
  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({5, 2, 2}));

  std::vector<TypeParam> backprop_error = conv.Backward({inputData}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 3);
  ASSERT_EQ(backprop_error[0].shape()[1], 5);
  ASSERT_EQ(backprop_error[0].shape()[2], 5);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(Convolution2DTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({3, 5, 5}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::Convolution2D<TypeParam>> conv("Convolution2D", 5u,
                                                                               3u, 3u, 2u);
  conv.AddInput(placeholder);

  TypeParam prediction = conv.Evaluate();

  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 2);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(Convolution2DTest, node_backward_test)  // Use the class as a Node
{
  TypeParam data({3, 5, 5});
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::Convolution2D<TypeParam>> conv("Convolution2D", 5u,
                                                                               3u, 3u, 2u);
  conv.AddInput(placeholder);
  TypeParam prediction = conv.Evaluate();

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({5, 2, 2}));
  auto      backprop_error = conv.BackPropagate(error_signal);

  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].second.shape().size(), 3);
  ASSERT_EQ(backprop_error[0].second.shape()[0], 3);
  ASSERT_EQ(backprop_error[0].second.shape()[1], 5);
  ASSERT_EQ(backprop_error[0].second.shape()[2], 5);
}

TYPED_TEST(Convolution2DTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution2D<TypeParam>>("Convolution2D", {"Input"}, 5u,
                                                                  3u, 3u, 2u);

  TypeParam data({3, 5, 5});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("Convolution2D");
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 2);
  ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(Convolution2DTest, getStateDict)
{
  fetch::ml::layers::Convolution2D<TypeParam> conv(
      5, 3, 3, 2, fetch::ml::details::ActivationType::NOTHING, "ConvTest");
  fetch::ml::StateDict<TypeParam> sd = conv.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);

  ASSERT_NE(sd.dict_["ConvTest_Weights"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["ConvTest_Weights"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({5u, 3u, 3u, 3u}));
}
