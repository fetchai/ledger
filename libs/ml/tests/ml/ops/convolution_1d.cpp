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

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

#include <vector>

template <typename T>
class Convolution1DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(Convolution1DTest, MyTypes);

TYPED_TEST(Convolution1DTest, forward_1x1x2_1x1x1x2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType input({1, 1, 2});
  ArrayType weights({1, 1, 1, 1});
  input.At(0, 0, 0)      = DataType{5};
  input.At(0, 0, 1)      = DataType{6};
  weights.At(0, 0, 0, 0) = DataType{-4};
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}));
  c.Forward({std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 2}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{-20});
  EXPECT_EQ(output.At(0, 0, 1), DataType{-24});
}

TYPED_TEST(Convolution1DTest, forward_1x3x1_1x1x3x1)
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

  ArrayType output(c.ComputeOutputShape(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}));
  c.Forward({std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0), DataType{5});
}

TYPED_TEST(Convolution1DTest, forward_3x3x1_5x3x3x1)
{
  // using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input({3, 3, 1});
  ArrayType                                weights({5, 3, 3, 1});
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}));
  c.Forward({std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({5, 1, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5x1_1x1x3x1)
{
  // using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input({1, 5, 1});
  ArrayType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<ArrayType> c;

  ArrayType output(c.ComputeOutputShape(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}));
  c.Forward({std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 3, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5x1_1x1x3x1_stride_2)
{
  //  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input({1, 5, 1});
  ArrayType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<ArrayType> c(2);

  ArrayType output(c.ComputeOutputShape(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}));
  c.Forward({std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 2, 1}));
}

TYPED_TEST(Convolution1DTest, forward_1x5x2_1x1x3x2_stride_2)
{
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType                                input({1, 5, 2});
  ArrayType                                weights({1, 1, 3, 1});
  fetch::ml::ops::Convolution1D<ArrayType> c(2);

  ArrayType output(c.ComputeOutputShape(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}));
  c.Forward({std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 2, 2}));
}

TYPED_TEST(Convolution1DTest, forward_3x3x2_5x3x3x2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 4;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 2;
  SizeType const batch_size      = 2;

  ArrayType input({input_channels, input_height, batch_size});
  ArrayType kernels({output_channels, input_channels, kernel_height, 1});
  ArrayType gt({output_channels, output_height, batch_size});

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        input(i_ic, i_i, i_b) = static_cast<DataType>(i_i + i_b);
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
        kernels(i_oc, i_ic, i_k, 0) = static_cast<DataType>(i_oc + 1);
      }
    }
  }

  fetch::ml::ops::Convolution1D<ArrayType> op;
  ArrayType                                output(op.ComputeOutputShape(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(kernels)}));
  op.Forward({std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(kernels)}, output);

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
  ASSERT_TRUE(output.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(Convolution1DTest, backward_3x3x2_5x3x3x2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;
  SizeType const input_height    = 3;
  SizeType const kernel_height   = 3;
  SizeType const output_height   = 1;
  SizeType const batch_size      = 2;

  ArrayType input({input_channels, input_height, batch_size});
  ArrayType kernels({output_channels, input_channels, kernel_height, 1});
  ArrayType error({output_channels, output_height, batch_size});
  ArrayType gt1(input.shape());
  ArrayType gt2(kernels.shape());

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        input(i_ic, i_i, i_b) = static_cast<DataType>(i_i + 1);
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
        gt2(i_oc, i_ic, i_k, 0)     = static_cast<DataType>((i_k + 1) * 2);
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
        error(i_oc, i_o, i_b) = static_cast<DataType>(i_o + 1);
      }
    }
  }

  fetch::ml::ops::Convolution1D<ArrayType> op;
  std::vector<ArrayType>                   prediction = op.Backward(
      {std::make_shared<ArrayType>(input), std::make_shared<ArrayType>(kernels)}, error);

  // Test correct gradient shape
  ASSERT_EQ(prediction.at(0).shape(), input.shape());
  ASSERT_EQ(prediction.at(1).shape(), kernels.shape());

  // Test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt1, DataType{1e-5f}, DataType{1e-5f}));
  ASSERT_TRUE(prediction[1].AllClose(gt2, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(Convolution1DTest, saveparams_test)
{
  using ArrayType     = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<ArrayType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Convolution1D<ArrayType>::SPType;
  using OpType        = typename fetch::ml::ops::Convolution1D<ArrayType>;

  ArrayType input({1, 1, 2});
  ArrayType weights({1, 1, 1, 1});
  input.At(0, 0, 0)      = DataType{5};
  input.At(0, 0, 1)      = DataType{6};
  weights.At(0, 0, 0, 0) = DataType{-4};

  OpType op;

  ArrayType     prediction(op.ComputeOutputShape(
      {std::make_shared<const ArrayType>(input), std::make_shared<const ArrayType>(weights)}));
  VecTensorType vec_data(
      {std::make_shared<const ArrayType>(input), std::make_shared<const ArrayType>(weights)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParamsInterface> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::ByteArrayBuffer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  ArrayType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const ArrayType>(input), std::make_shared<const ArrayType>(weights)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                      fetch::math::function_tolerance<DataType>()));
}
