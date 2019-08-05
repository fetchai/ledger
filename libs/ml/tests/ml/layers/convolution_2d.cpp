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
#include "ml/layers/convolution_2d.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class Convolution2DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(Convolution2DTest, MyTypes);

TYPED_TEST(Convolution2DTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(
      std::vector<typename TypeParam::SizeType>({input_channels, input_height, input_width, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {

        input.Set(i_ic, i_i, j_i, 0, static_cast<DataType>(i_i * j_i + 1));
      }
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution2D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);
  conv.SetInput("Conv2D_Input", input);
  TypeParam output = conv.Evaluate("Conv2D_Conv2D", true);

  // test correct values
  ASSERT_EQ(output.shape().size(), 4);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 1);
  ASSERT_EQ(output.shape()[2], 1);
  ASSERT_EQ(output.shape()[3], 1);

  ArrayType gt({output_channels, output_height, output_width});
  gt.Set(0, 0, 0, static_cast<DataType>(1.1533032542));
  gt.Set(1, 0, 0, static_cast<DataType>(-7.7671483948));
  gt.Set(2, 0, 0, static_cast<DataType>(-4.0066583846));
  gt.Set(3, 0, 0, static_cast<DataType>(-7.9669202564));
  gt.Set(4, 0, 0, static_cast<DataType>(-16.5230417126));

  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution2DTest, ops_forward_test)  // Use the class as an Ops
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(
      std::vector<typename TypeParam::SizeType>({input_channels, input_height, input_width, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {

        input.Set(i_ic, i_i, j_i, 0, static_cast<DataType>(i_i * j_i + 1));
      }
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution2D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);

  ArrayType output(conv.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  conv.Forward({std::make_shared<TypeParam>(input)}, output);

  // test correct values
  ASSERT_EQ(output.shape().size(), 4);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 1);
  ASSERT_EQ(output.shape()[2], 1);
  ASSERT_EQ(output.shape()[3], 1);

  ArrayType gt({output_channels, output_height, output_width, 1});
  gt.Set(0, 0, 0, 0, static_cast<DataType>(1.1533032542));
  gt.Set(1, 0, 0, 0, static_cast<DataType>(-7.7671483948));
  gt.Set(2, 0, 0, 0, static_cast<DataType>(-4.0066583846));
  gt.Set(3, 0, 0, 0, static_cast<DataType>(-7.9669202564));
  gt.Set(4, 0, 0, 0, static_cast<DataType>(-16.5230417126));

  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution2DTest, ops_backward_test)  // Use the class as an Ops
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(
      std::vector<typename TypeParam::SizeType>({input_channels, input_height, input_width, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {
        input.Set(i_ic, i_i, j_i, 0, static_cast<DataType>(i_i * j_i + 1));
      }
    }
  }

  // Generate error
  ArrayType error_signal(
      std::vector<typename TypeParam::SizeType>({output_channels, output_height, output_width, 1}));

  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      for (SizeType j_o{0}; j_o < output_width; ++j_o)
      {
        error_signal.Set(i_oc, i_o, j_o, 0, static_cast<DataType>(2));
      }
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution2D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);

  ArrayType output(conv.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  conv.Forward({std::make_shared<TypeParam>(input)}, output);

  std::vector<TypeParam> backprop_error =
      conv.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  // test correct values
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 4);
  ASSERT_EQ(backprop_error[0].shape()[0], input_channels);
  ASSERT_EQ(backprop_error[0].shape()[1], input_height);
  ASSERT_EQ(backprop_error[0].shape()[2], input_width);
  ASSERT_EQ(backprop_error[0].shape()[3], 1);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 0, 0, 0)), -4.3077492713928222656);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 0, 0, 0)), 9.162715911865234375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 0, 0, 0)), 0.80360949039459228516);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 1, 0, 0)), 1.2491617202758789062);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 1, 0, 0)), 2.8053097724914550781);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 1, 0, 0)), -4.166011810302734375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 2, 0, 0)), 2.4086174964904785156);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 2, 0, 0)), -0.86411559581756591797);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 2, 0, 0)), -3.5623354911804199219);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 0, 1, 0)), -2.9907839298248291016);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 0, 1, 0)), -0.16291338205337524414);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 0, 1, 0)), -2.5308477878570556641);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 1, 1, 0)), -1.2312210798263549805);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 1, 1, 0)), -6.6115474700927734375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 1, 1, 0)), 3.2868711948394775391);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 2, 1, 0)), -4.994899749755859375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 2, 1, 0)), -2.9489955902099609375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 2, 1, 0)), -2.4173920154571533203);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 0, 2, 0)), 2.4823324680328369141);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 0, 2, 0)), 2.4479858875274658203);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 0, 2, 0)), -0.3612575531005859375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 1, 2, 0)), -6.4253511428833007812);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 1, 2, 0)), -3.184307098388671875);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 1, 2, 0)), 0.51499307155609130859);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 2, 2, 0)), -1.5936613082885742188);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 2, 2, 0)), -0.41774189472198486328);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 2, 2, 0)), 0.98040378093719482422);
}

TYPED_TEST(Convolution2DTest, node_forward_test)  // Use the class as a Node
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(
      std::vector<typename TypeParam::SizeType>({input_channels, input_height, input_width, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {

        input.Set(i_ic, i_i, j_i, 0, static_cast<DataType>(i_i * j_i + 1));
      }
    }
  }

  // Evaluate
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(input);

  fetch::ml::Node<TypeParam, fetch::ml::layers::Convolution2D<TypeParam>> conv(
      "Convolution2D", output_channels, input_channels, kernel_height, stride_size);
  conv.AddInput(placeholder);

  TypeParam prediction = *conv.Evaluate(true);

  // test correct values
  ASSERT_EQ(prediction.shape().size(), 4);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 1);
  ASSERT_EQ(prediction.shape()[2], 1);
  ASSERT_EQ(prediction.shape()[3], 1);

  ArrayType gt({output_channels, output_height, output_width, 1});
  gt.Set(0, 0, 0, 0, static_cast<DataType>(1.1533032542));
  gt.Set(1, 0, 0, 0, static_cast<DataType>(-7.7671483948));
  gt.Set(2, 0, 0, 0, static_cast<DataType>(-4.0066583846));
  gt.Set(3, 0, 0, 0, static_cast<DataType>(-7.9669202564));
  gt.Set(4, 0, 0, 0, static_cast<DataType>(-16.5230417126));

  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution2DTest, node_backward_test)  // Use the class as a Node
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(
      std::vector<typename TypeParam::SizeType>({input_channels, input_height, input_width, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {
        input.Set(i_ic, i_i, j_i, 0, static_cast<DataType>(i_i * j_i + 1));
      }
    }
  }

  // Generate error
  ArrayType error_signal(
      std::vector<typename TypeParam::SizeType>({output_channels, output_height, output_width, 1}));

  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      for (SizeType j_o{0}; j_o < output_width; ++j_o)
      {
        error_signal.Set(i_oc, i_o, j_o, 0, static_cast<DataType>(2));
      }
    }
  }

  // Evaluate
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(input);

  fetch::ml::Node<TypeParam, fetch::ml::layers::Convolution2D<TypeParam>> conv(
      "Convolution2D", output_channels, input_channels, kernel_height, stride_size);
  conv.AddInput(placeholder);
  TypeParam prediction     = *conv.Evaluate(true);
  auto      backprop_error = conv.BackPropagateSignal(error_signal);

  // test correct values
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].second.shape().size(), 4);
  ASSERT_EQ(backprop_error[0].second.shape()[0], input_channels);
  ASSERT_EQ(backprop_error[0].second.shape()[1], input_height);
  ASSERT_EQ(backprop_error[0].second.shape()[2], input_width);
  ASSERT_EQ(backprop_error[0].second.shape()[3], 1);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 0, 0, 0)),
                  -4.3077492713928222656);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 0, 0, 0)),
                  9.162715911865234375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 0, 0, 0)),
                  0.80360949039459228516);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 1, 0, 0)),
                  1.2491617202758789062);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 1, 0, 0)),
                  2.8053097724914550781);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 1, 0, 0)),
                  -4.166011810302734375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 2, 0, 0)),
                  2.4086174964904785156);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 2, 0, 0)),
                  -0.86411559581756591797);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 2, 0, 0)),
                  -3.5623354911804199219);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 0, 1, 0)),
                  -2.9907839298248291016);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 0, 1, 0)),
                  -0.16291338205337524414);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 0, 1, 0)),
                  -2.5308477878570556641);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 1, 1, 0)),
                  -1.2312210798263549805);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 1, 1, 0)),
                  -6.6115474700927734375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 1, 1, 0)),
                  3.2868711948394775391);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 2, 1, 0)),
                  -4.994899749755859375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 2, 1, 0)),
                  -2.9489955902099609375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 2, 1, 0)),
                  -2.4173920154571533203);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 0, 2, 0)),
                  2.4823324680328369141);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 0, 2, 0)),
                  2.4479858875274658203);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 0, 2, 0)),
                  -0.3612575531005859375);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 1, 2, 0)),
                  -6.4253511428833007812);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 1, 2, 0)),
                  -3.184307098388671875);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 1, 2, 0)),
                  0.51499307155609130859);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 2, 2, 0)),
                  -1.5936613082885742188);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 2, 2, 0)),
                  -0.41774189472198486328);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 2, 2, 0)),
                  0.98040378093719482422);
}

TYPED_TEST(Convolution2DTest, graph_forward_test)  // Use the class as a Node
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const input_width     = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const output_width    = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(
      std::vector<typename TypeParam::SizeType>({input_channels, input_height, input_width, 1}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {

        input.Set(i_ic, i_i, j_i, 0, static_cast<DataType>(i_i * j_i + 1));
      }
    }
  }

  // Evaluate
  fetch::ml::Graph<TypeParam> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution2D<TypeParam>>(
      "Convolution2D", {"Input"}, output_channels, input_channels, kernel_height, stride_size);
  g.SetInput("Input", input);

  TypeParam prediction = g.Evaluate("Convolution2D", true);

  // test correct values
  ASSERT_EQ(prediction.shape().size(), 4);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 1);
  ASSERT_EQ(prediction.shape()[2], 1);
  ASSERT_EQ(prediction.shape()[3], 1);

  ArrayType gt({output_channels, output_height, output_width, 1});
  gt.Set(0, 0, 0, 0, static_cast<DataType>(1.1533032542));
  gt.Set(1, 0, 0, 0, static_cast<DataType>(-7.7671483948));
  gt.Set(2, 0, 0, 0, static_cast<DataType>(-4.0066583846));
  gt.Set(3, 0, 0, 0, static_cast<DataType>(-7.9669202564));
  gt.Set(4, 0, 0, 0, static_cast<DataType>(-16.5230417126));

  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution2DTest, getStateDict)
{
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Initialise weights
  fetch::ml::layers::Convolution2D<ArrayType> conv(
      output_channels, input_channels, kernel_height, stride_size,
      fetch::ml::details::ActivationType::NOTHING, "ConvTest");
  fetch::ml::StateDict<ArrayType> sd = conv.StateDict();

  // Get weights
  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);
  auto weights_ptr = sd.dict_["ConvTest_Weights"].weights_;

  // Test correct values
  ASSERT_NE(weights_ptr, nullptr);
  EXPECT_EQ(weights_ptr->shape(), std::vector<SizeType>({output_channels, input_channels,
                                                         kernel_height, kernel_height, 1}));

  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(0, 0, 0, 0, 0)), -0.970493f);
  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(1, 1, 1, 1, 0)), -0.85325855f);
  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(4, 2, 2, 2, 0)), -0.096136682f);
}
