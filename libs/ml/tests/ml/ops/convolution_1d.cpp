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

#include "ml/ops/convolution_1d.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <gtest/gtest.h>

template <typename T>
class Convolution1DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(Convolution1DTest, MyTypes);

TYPED_TEST(Convolution1DTest, forward_1x1_1x1x1)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType input({1, 1, 1});
  ArrayType weights({1, 1, 1, 1});
  input.At(0, 0, 0)      = DataType{5};
  weights.At(0, 0, 0, 0) = DataType{-4};
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{-20});
}

TYPED_TEST(Convolution1DTest, forward_1x3_1x1x3)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType input({1, 3, 1});
  ArrayType weights({1, 1, 3, 1});
  for (SizeType i{0}; i < 3; ++i)
  {
    input.At(0, i, 0)      = static_cast<DataType>(i);
    weights.At(0, 0, i, 0) = static_cast<DataType>(i);
  }
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{5});
}

TYPED_TEST(Convolution1DTest, forward_3x3_1x3x3)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType input({3, 3, 1});
  ArrayType weights({1, 3, 3, 1});
  SizeType  counter = 0;
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType k{0}; k < 3; ++k)
    {
      input.At(i, k, 0)      = static_cast<DataType>(counter);
      weights.At(0, i, k, 0) = static_cast<DataType>(counter);
      ++counter;
    }
  }
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{204});
}

TYPED_TEST(Convolution1DTest, forward_3x3_5x3x3)
{
  // using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input({3, 3, 1});
  ArrayType                                weights({5, 3, 3, 1});
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({5, 1, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5_1x1x3)
{
  // using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input({1, 5, 1});
  ArrayType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 3, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5_1x1x3_stride_2)
{
  //  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input({1, 5, 1});
  ArrayType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<ArrayType> c(2);

  ArrayType output(c.ComputeOutputShape({input, weights}));
  c.Forward({input, weights}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 2, 1}));
}

TYPED_TEST(Convolution1DTest, backward_3x3_5x3x3)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;

  ArrayType input({input_channels, input_height, 1});
  ArrayType kernels({output_channels, input_channels, kernel_height, 1});
  ArrayType error({output_channels, output_height, 1});
  ArrayType gt1(input.shape());
  ArrayType gt2(kernels.shape());

  // Generate input
  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
  {
    for (SizeType i_i{0}; i_i < input_height; ++i_i)
    {
      input.Set(i_ic, i_i, 0, static_cast<DataType>(i_i + 1));
      gt1.Set(i_ic, i_i, 0, DataType{10});
    }
  }

  // Generate kernels
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_k{0}; i_k < kernel_height; ++i_k)
      {

        kernels.Set(i_oc, i_ic, i_k, 0, DataType{2});
        gt2.Set(i_oc, i_ic, i_k, 0, static_cast<DataType>(i_k + 1));
      }
    }
  }

  // Generate error signal
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
  {
    for (SizeType i_o{0}; i_o < output_height; ++i_o)
    {

      error.Set(i_oc, i_o, 0, static_cast<DataType>(i_o + 1));
    }
  }

  fetch::ml::ops::Convolution1D<ArrayType> op;
  std::vector<ArrayType>                   prediction = op.Backward({input, kernels}, error);

  // Test correct gradient shape
  ASSERT_EQ(prediction.at(0).shape(), input.shape());
  ASSERT_EQ(prediction.at(1).shape(), kernels.shape());

  // Test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt1, DataType{1e-5f}, DataType{1e-5f}));
  ASSERT_TRUE(prediction[1].AllClose(gt2, DataType{1e-5f}, DataType{1e-5f}));
}
