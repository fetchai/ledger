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

#include "ml/layers/convolution_1d.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class Convolution1DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(Convolution1DTest, MyTypes);

TYPED_TEST(Convolution1DTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(std::vector<typename TypeParam::SizeType>({input_channels, input_height}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input.Set(i_ic, i_i, static_cast<DataType>(i_i + 1));
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution1D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);
  conv.SetInput("Conv1D_Input", input);
  TypeParam output = conv.Evaluate("Conv1D_Conv1D");

  // test correct values
  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 1);

  ArrayType gt({output_channels, output_height});
  gt.Set(0, 0, static_cast<DataType>(-4.28031352977));
  gt.Set(1, 0, static_cast<DataType>(-4.03654768132));
  gt.Set(2, 0, static_cast<DataType>(8.11192789580));
  gt.Set(3, 0, static_cast<DataType>(1.763717529829592));
  gt.Set(4, 0, static_cast<DataType>(-1.8677866039798));

  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, ops_forward_test)  // Use the class as an Ops
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  ArrayType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input.Set(i_ic, i_i, static_cast<DataType>(i_i + 1));
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution1D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);
  ArrayType output = conv.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({input}));

  // test correct values
  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 5);
  ASSERT_EQ(output.shape()[1], 1);

  ArrayType gt({output_channels, output_height});
  gt.Set(0, 0, static_cast<DataType>(-4.28031352977));
  gt.Set(1, 0, static_cast<DataType>(-4.03654768132));
  gt.Set(2, 0, static_cast<DataType>(8.11192789580));
  gt.Set(3, 0, static_cast<DataType>(1.763717529829592));
  gt.Set(4, 0, static_cast<DataType>(-1.8677866039798));

  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, ops_backward_test)  // Use the class as an Ops
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  ArrayType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input.Set(i_ic, i_i, static_cast<DataType>(i_i + 1));
    }
  }

  // Generate error
  ArrayType error_signal(
      std::vector<typename TypeParam::SizeType>({output_channels, output_height}));

  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      error_signal.Set(i_oc, i_o, static_cast<DataType>(2));
    }
  }

  // Evaluate
  fetch::ml::layers::Convolution1D<TypeParam> conv(output_channels, input_channels, kernel_height,
                                                   stride_size);
  TypeParam output = conv.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({input}));
  std::vector<TypeParam> backprop_error = conv.Backward({input}, error_signal);

  // test correct values
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 2);
  ASSERT_EQ(backprop_error[0].shape()[0], input_channels);
  ASSERT_EQ(backprop_error[0].shape()[1], input_height);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(0, 0)), -4.30774927f);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(1, 0)), 9.1627159f);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(2, 0)), 0.80360967f);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).At(3, 0)), 1.2491617f);
}

TYPED_TEST(Convolution1DTest, node_forward_test)  // Use the class as a Node
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  TypeParam input(std::vector<typename TypeParam::SizeType>({input_channels, input_height}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input.Set(i_ic, i_i, static_cast<DataType>(i_i + 1));
    }
  }

  // Evaluate
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(input);

  fetch::ml::Node<TypeParam, fetch::ml::layers::Convolution1D<TypeParam>> conv(
      "Convolution1D", output_channels, input_channels, kernel_height, stride_size);
  conv.AddInput(placeholder);

  TypeParam prediction = conv.Evaluate();

  // test correct values
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 1);

  ArrayType gt({output_channels, output_height});
  gt.Set(0, 0, static_cast<DataType>(-4.28031352977));
  gt.Set(1, 0, static_cast<DataType>(-4.03654768132));
  gt.Set(2, 0, static_cast<DataType>(8.11192789580));
  gt.Set(3, 0, static_cast<DataType>(1.763717529829592));
  gt.Set(4, 0, static_cast<DataType>(-1.8677866039798));

  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, node_backward_test)  // Use the class as a Node
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  ArrayType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input.Set(i_ic, i_i, static_cast<DataType>(i_i + 1));
    }
  }

  // Generate error
  ArrayType error_signal(
      std::vector<typename TypeParam::SizeType>({output_channels, output_height}));

  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      error_signal.Set(i_oc, i_o, static_cast<DataType>(2));
    }
  }

  // Evaluate
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(input);

  fetch::ml::Node<TypeParam, fetch::ml::layers::Convolution1D<TypeParam>> conv(
      "Convolution1D", output_channels, input_channels, kernel_height, stride_size);
  conv.AddInput(placeholder);
  TypeParam prediction     = conv.Evaluate();
  auto      backprop_error = conv.BackPropagate(error_signal);

  // test correct values
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].second.shape().size(), 2);
  ASSERT_EQ(backprop_error[0].second.shape()[0], input_channels);
  ASSERT_EQ(backprop_error[0].second.shape()[1], input_height);

  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(0, 0)), -4.30774927f);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(1, 0)), 9.1627159f);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(2, 0)), 0.80360967f);
  EXPECT_FLOAT_EQ(static_cast<float>(backprop_error.at(0).second.At(3, 0)), 1.2491617f);
}

TYPED_TEST(Convolution1DTest, graph_forward_test)  // Use the class as a Node
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const stride_size     = 1;

  // Generate input
  ArrayType input(std::vector<typename TypeParam::SizeType>({input_channels, input_height}));

  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input.Set(i_ic, i_i, static_cast<DataType>(i_i + 1));
    }
  }

  // Evaluate
  fetch::ml::Graph<TypeParam> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::Convolution1D<TypeParam>>(
      "Convolution1D", {"Input"}, output_channels, input_channels, kernel_height, stride_size);
  g.SetInput("Input", input);

  TypeParam prediction = g.Evaluate("Convolution1D");

  // test correct values
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 5);
  ASSERT_EQ(prediction.shape()[1], 1);

  ArrayType gt({output_channels, output_height});
  gt.Set(0, 0, static_cast<DataType>(-4.28031352977));
  gt.Set(1, 0, static_cast<DataType>(-4.03654768132));
  gt.Set(2, 0, static_cast<DataType>(8.11192789580));
  gt.Set(3, 0, static_cast<DataType>(1.763717529829592));
  gt.Set(4, 0, static_cast<DataType>(-1.8677866039798));

  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}

TYPED_TEST(Convolution1DTest, getStateDict)
{
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const kernel_height   = 3;
  SizeType const stride_size     = 1;

  // Initialize weights
  fetch::ml::layers::Convolution1D<ArrayType> conv(
      output_channels, input_channels, kernel_height, stride_size,
      fetch::ml::details::ActivationType::NOTHING, "ConvTest");
  fetch::ml::StateDict<ArrayType> sd = conv.StateDict();

  // Get weights
  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 1);
  auto weights_ptr = sd.dict_["ConvTest_Weights"].weights_;

  // Test correct values
  ASSERT_NE(weights_ptr, nullptr);
  EXPECT_EQ(weights_ptr->shape(),
            std::vector<SizeType>({output_channels, input_channels, kernel_height}));

  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(0, 0, 0)), -0.970493f);
  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(1, 1, 1)), 0.55109727f);
  EXPECT_FLOAT_EQ(static_cast<float>(weights_ptr->At(4, 2, 2)), -0.97583634f);
}
