//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/base_types.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

template <typename T>
class Convolution1DTest : public ::testing::Test
{
};

TYPED_TEST_CASE(Convolution1DTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(Convolution1DTest, forward_1x1x2_1x1x1x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType input({1, 1, 2});
  TensorType weights({1, 1, 1, 1});
  input.At(0, 0, 0)      = DataType{5};
  input.At(0, 0, 1)      = DataType{6};
  weights.At(0, 0, 0, 0) = DataType{-4};
  fetch::ml::ops::Convolution1D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 2}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{-20});
  EXPECT_EQ(output.At(0, 0, 1), DataType{-24});
}

TYPED_TEST(Convolution1DTest, forward_1x3x1_1x1x3x1)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType input({1, 3, 1});
  TensorType weights({1, 1, 3, 1});
  for (SizeType i{0}; i < 3; ++i)
  {
    input.At(0, i, 0)      = fetch::math::AsType<DataType>(i);
    weights.At(0, 0, i, 0) = fetch::math::AsType<DataType>(i);
  }
  fetch::ml::ops::Convolution1D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{5});
}

TYPED_TEST(Convolution1DTest, forward_3x3x1_5x3x3x1)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType                                input({3, 3, 1});
  TensorType                                weights({5, 3, 3, 1});
  fetch::ml::ops::Convolution1D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({5, 1, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5x1_1x1x3x1)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType                                input({1, 5, 1});
  TensorType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 3, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5x1_1x1x3x1_stride_2)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType                                input({1, 5, 1});
  TensorType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<TensorType> c(2);

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 2, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5x2_1x1x3x2_stride_2)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType                                input({1, 5, 2});
  TensorType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<TensorType> c(2);

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 2, 2}));
}

TYPED_TEST(Convolution1DTest, forward_3x3x2_5x3x3x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 4;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 2;
  SizeType const batch_size      = 2;

  TensorType input({input_channels, input_height, batch_size});
  TensorType kernels({output_channels, input_channels, kernel_height, 1});
  TensorType gt({output_channels, output_height, batch_size});

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        input(i_ic, i_i, i_b) = fetch::math::AsType<DataType>(i_i + i_b);
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
        kernels(i_oc, i_ic, i_k, 0) = fetch::math::AsType<DataType>(i_oc + 1);
      }
    }
  }

  fetch::ml::ops::Convolution1D<TensorType> op;
  TensorType                                output(op.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}));
  op.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, output);

  // Generate gt
  gt(0, 0, 0) = DataType{9};
  gt(0, 1, 0) = DataType{18};
  gt(1, 0, 0) = DataType{18};
  gt(1, 1, 0) = DataType{36};
  gt(2, 0, 0) = DataType{27};
  gt(2, 1, 0) = DataType{54};
  gt(3, 0, 0) = DataType{36};
  gt(3, 1, 0) = DataType{72};
  gt(4, 0, 0) = DataType{45};
  gt(4, 1, 0) = DataType{90};
  gt(0, 0, 1) = DataType{18};
  gt(0, 1, 1) = DataType{27};
  gt(1, 0, 1) = DataType{36};
  gt(1, 1, 1) = DataType{54};
  gt(2, 0, 1) = DataType{54};
  gt(2, 1, 1) = DataType{81};
  gt(3, 0, 1) = DataType{72};
  gt(3, 1, 1) = DataType{108};
  gt(4, 0, 1) = DataType{90};
  gt(4, 1, 1) = DataType{135};

  // Test correct gradient shape
  ASSERT_EQ(output.shape(), gt.shape());

  // Test correct values
  ASSERT_TRUE(output.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(Convolution1DTest, backward_3x3x2_5x3x3x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const batch_size      = 2;

  TensorType input({input_channels, input_height, batch_size});
  TensorType kernels({output_channels, input_channels, kernel_height, 1});
  TensorType error({output_channels, output_height, batch_size});
  TensorType gt1(input.shape());
  TensorType gt2(kernels.shape());

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        input(i_ic, i_i, i_b) = fetch::math::AsType<DataType>(i_i + 1);
        gt1(i_ic, i_i, i_b)   = DataType{10};
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

        kernels(i_oc, i_ic, i_k, 0) = DataType{2};
        gt2(i_oc, i_ic, i_k, 0)     = fetch::math::AsType<DataType>((i_k + 1) * 2);
      }
    }
  }

  // Generate error signal
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)
    {
      for (SizeType i_o{0}; i_o < output_height; ++i_o)
      {
        error(i_oc, i_o, i_b) = fetch::math::AsType<DataType>(i_o + 1);
      }
    }
  }

  fetch::ml::ops::Convolution1D<TensorType> op;
  std::vector<TensorType>                   prediction = op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  // Test correct gradient shape
  ASSERT_EQ(prediction.at(0).shape(), input.shape());
  ASSERT_EQ(prediction.at(1).shape(), kernels.shape());

  // Test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

}  // namespace
