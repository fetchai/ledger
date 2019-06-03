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
#include "ml/ops/convolution_2d.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include <gtest/gtest.h>

template <typename T>
class Convolution2DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(Convolution2DTest, MyTypes);

TYPED_TEST(Convolution2DTest, forward_1x1x1_1x1x1x1)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType input({1, 1, 1});
  ArrayType weights({1, 1, 1, 1});
  input.At(0, 0, 0)      = DataType{5};
  weights.At(0, 0, 0, 0) = DataType{-4};
  fetch::ml::ops::Convolution2D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{-20});
}

TYPED_TEST(Convolution2DTest, forward_1x3x3_1x1x3x3)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType input({1, 3, 3});
  ArrayType weights({1, 1, 3, 3});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      input.At(0, i, j)      = static_cast<DataType>((i * 3) + j);
      weights.At(0, 0, i, j) = static_cast<DataType>((i * 3) + j);
    }
  }
  fetch::ml::ops::Convolution2D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{204});
}

TYPED_TEST(Convolution2DTest, forward_3x3x3_1x3x3x3)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType input({3, 3, 3});
  ArrayType weights({1, 3, 3, 3});
  SizeType  counter = 0;
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 3; ++k)
      {
        input.At(i, j, k)      = static_cast<DataType>(counter);
        weights.At(0, i, j, k) = static_cast<DataType>(counter);
        ++counter;
      }
    }
  }
  fetch::ml::ops::Convolution2D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{6201});
}

TYPED_TEST(Convolution2DTest, forward_3x3x3_5x3x3x3)
{
  // using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input(std::vector<SizeType>({3, 3, 3}));
  ArrayType                                weights(std::vector<SizeType>({5, 3, 3, 3}));
  fetch::ml::ops::Convolution2D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({5, 1, 1}));
}

TYPED_TEST(Convolution2DTest, forward_1x5x5_1x1x3x3)
{
  // using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input(std::vector<SizeType>({1, 5, 5}));
  ArrayType                                weights(std::vector<SizeType>({1, 1, 3, 3}));
  fetch::ml::ops::Convolution2D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 3, 3}));
}

TYPED_TEST(Convolution2DTest, forward_1x5x5_1x1x3x3_stride_2)
{
  // using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input(std::vector<SizeType>({1, 5, 5}));
  ArrayType                                weights(std::vector<SizeType>({1, 1, 3, 3}));
  fetch::ml::ops::Convolution2D<ArrayType> c(2);

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 2, 2}));
}

TYPED_TEST(Convolution2DTest, backward_3x3x3_5x3x3x3)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_width     = 3;
  SizeType const input_height    = 3;

  SizeType const kernel_width  = 3;
  SizeType const kernel_height = 3;

  SizeType const output_width  = 1;
  SizeType const output_height = 1;

  ArrayType input({input_channels, input_height, input_width});
  ArrayType kernels({output_channels, input_channels, kernel_height, kernel_width});
  ArrayType error({output_channels, output_width, output_height});
  ArrayType gt1(input.shape());
  ArrayType gt2(kernels.shape());

  // Generate input
  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {
        input.Set(i_ic, i_i, j_i, static_cast<DataType>((i_i + 1) * (j_i + 1)));
        gt1.Set(i_ic, i_i, j_i, DataType{10});
      }
    }
  }

  // Generate kernels
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_k{0}; i_k < kernel_height; ++i_k)
      {
        for (SizeType j_k{0}; j_k < kernel_width; ++j_k)
        {
          kernels.Set(i_oc, i_ic, i_k, j_k, DataType{2});
          gt2.Set(i_oc, i_ic, i_k, j_k, static_cast<DataType>((i_k + 1) * (j_k + 1)));
        }
      }
    }
  }

  // Generate error signal
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      for (SizeType j_o{0}; j_o < output_width; ++j_o)
      {
        error.Set(i_oc, i_o, j_o, static_cast<DataType>((i_o + 1) * (j_o + 1)));
      }
    }
  }

  fetch::ml::ops::Convolution2D<ArrayType> op;
  std::vector<ArrayType>                   prediction = op.Backward({input, kernels}, error);

  // Test correct gradient shape
  ASSERT_EQ(prediction.at(0).shape(), input.shape());
  ASSERT_EQ(prediction.at(1).shape(), kernels.shape());

  // Test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt1, DataType{1e-5f}, DataType{1e-5f}));
  ASSERT_TRUE(prediction[1].AllClose(gt2, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(Convolution2DTest, backward_3x3x2_5x3x3x2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_width     = 3;
  SizeType const input_height    = 2;

  SizeType const kernel_width  = 3;
  SizeType const kernel_height = 2;

  SizeType const output_width  = 1;
  SizeType const output_height = 1;

  ArrayType input({input_channels, input_height, input_width});
  ArrayType kernels({output_channels, input_channels, kernel_height, kernel_width});
  ArrayType error({output_channels, output_width, output_height});
  ArrayType gt1(input.shape());
  ArrayType gt2(kernels.shape());

  // Generate input
  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      for (SizeType j_i{0}; j_i < input_width; ++j_i)
      {
        input.Set(i_ic, i_i, j_i, static_cast<DataType>((i_i + 1) * (j_i + 1)));
        gt1.Set(i_ic, i_i, j_i, DataType{10});
      }
    }
  }

  // Generate kernels
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_k{0}; i_k < kernel_height; ++i_k)
      {
        for (SizeType j_k{0}; j_k < kernel_width; ++j_k)
        {
          kernels.Set(i_oc, i_ic, i_k, j_k, DataType{2});
          gt2.Set(i_oc, i_ic, i_k, j_k, static_cast<DataType>((i_k + 1) * (j_k + 1)));
        }
      }
    }
  }

  // Generate error signal
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {
      for (SizeType j_o{0}; j_o < output_width; ++j_o)
      {
        error.Set(i_oc, i_o, j_o, static_cast<DataType>((i_o + 1) * (j_o + 1)));
      }
    }
  }

  fetch::ml::ops::Convolution2D<ArrayType> op;
  std::vector<ArrayType>                   prediction = op.Backward({input, kernels}, error);

  // Test correct gradient shape
  ASSERT_EQ(prediction.at(0).shape(), input.shape());
  ASSERT_EQ(prediction.at(1).shape(), kernels.shape());

  // Test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt1, DataType{1e-5f}, DataType{1e-5f}));
  ASSERT_TRUE(prediction[1].AllClose(gt2, DataType{1e-5f}, DataType{1e-5f}));
}
