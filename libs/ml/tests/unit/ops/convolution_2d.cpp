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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class Convolution2DTest : public ::testing::Test
{
};

TYPED_TEST_CASE(Convolution2DTest, math::test::TensorFloatingTypes);

TYPED_TEST(Convolution2DTest, forward_1x1x1x2_1x1x1x1x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType input({1, 1, 1, 2});
  TensorType weights({1, 1, 1, 1, 1});
  input.At(0, 0, 0, 0)      = DataType{5};
  input.At(0, 0, 0, 1)      = DataType{6};
  weights.At(0, 0, 0, 0, 0) = DataType{-4};
  fetch::ml::ops::Convolution2D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1, 2}));
  EXPECT_EQ(output.At(0, 0, 0, 0), DataType{-20});
  EXPECT_EQ(output.At(0, 0, 0, 1), DataType{-24});
}

TYPED_TEST(Convolution2DTest, forward_1x3x3x1_1x1x3x3x1)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType input({1, 3, 3, 1});
  TensorType weights({1, 1, 3, 3, 1});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      input.At(0, i, j, 0)      = fetch::math::AsType<DataType>((i * 3) + j);
      weights.At(0, 0, i, j, 0) = fetch::math::AsType<DataType>((i * 3) + j);
    }
  }
  fetch::ml::ops::Convolution2D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0, 0), DataType{204});
}

TYPED_TEST(Convolution2DTest, forward_3x3x3x1_1x3x3x3x1)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType input({3, 3, 3, 1});
  TensorType weights({1, 3, 3, 3, 1});
  SizeType   counter = 0;
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 3; ++k)
      {
        input.At(i, j, k, 0)      = fetch::math::AsType<DataType>(counter);
        weights.At(0, i, j, k, 0) = fetch::math::AsType<DataType>(counter);
        ++counter;
      }
    }
  }
  fetch::ml::ops::Convolution2D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 1, 1, 1}));
  EXPECT_EQ(output.At(0, 0, 0, 0), DataType{6201});
}

TYPED_TEST(Convolution2DTest, forward_3x3x3x1_5x3x3x3x1)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType                                input(std::vector<SizeType>({3, 3, 3, 1}));
  TensorType                                weights(std::vector<SizeType>({5, 3, 3, 3, 1}));
  fetch::ml::ops::Convolution2D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({5, 1, 1, 1}));
}

TYPED_TEST(Convolution2DTest, forward_1x5x5x3_1x1x3x3x3)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType                                input(std::vector<SizeType>({1, 5, 5, 3}));
  TensorType                                weights(std::vector<SizeType>({1, 1, 3, 3, 1}));
  fetch::ml::ops::Convolution2D<TensorType> c;

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 3, 3, 3}));
}

TYPED_TEST(Convolution2DTest, forward_1x5x5x3_1x1x3x3x3_stride_2)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType                                input(std::vector<SizeType>({1, 5, 5, 3}));
  TensorType                                weights(std::vector<SizeType>({1, 1, 3, 3, 3}));
  fetch::ml::ops::Convolution2D<TensorType> c(2);

  TensorType output(c.ComputeOutputShape(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}));
  c.Forward({std::make_shared<TensorType>(input), std::make_shared<TensorType>(weights)}, output);

  ASSERT_EQ(output.shape(), std::vector<SizeType>({1, 2, 2, 3}));
}

TYPED_TEST(Convolution2DTest, backward_3x3x3x2_5x3x3x3x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;

  SizeType const input_width  = 3;
  SizeType const input_height = 3;

  SizeType const kernel_width  = 3;
  SizeType const kernel_height = 3;

  SizeType const output_width  = 1;
  SizeType const output_height = 1;

  SizeType const batch_size = 2;

  TensorType input({input_channels, input_height, input_width, batch_size});
  TensorType kernels({output_channels, input_channels, kernel_height, kernel_width, 1});
  TensorType error({output_channels, output_height, output_width, batch_size});
  TensorType gt1(input.shape());
  TensorType gt2(kernels.shape());

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        for (SizeType j_i{0}; j_i < input_width; ++j_i)
        {
          input(i_ic, i_i, j_i, i_b) = fetch::math::AsType<DataType>(i_i + 1);
          gt1(i_ic, i_i, j_i, i_b)   = DataType{10};
        }
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
          kernels(i_oc, i_ic, i_k, j_k, 0) = DataType{2};
          gt2(i_oc, i_ic, i_k, j_k, 0)     = fetch::math::AsType<DataType>((i_k + 1) * 2);
        }
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
        for (SizeType j_o{0}; j_o < output_width; ++j_o)
        {
          error(i_oc, i_o, j_o, i_b) = static_cast<DataType>(i_o + 1);
        }
      }
    }
  }

  fetch::ml::ops::Convolution2D<TensorType> op;
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

TYPED_TEST(Convolution2DTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Convolution2D<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Convolution2D<TensorType>;
  using SizeType      = fetch::math::SizeType;

  TensorType input({3, 3, 3, 1});
  TensorType weights({1, 3, 3, 3, 1});
  SizeType   counter = 0;
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 3; ++k)
      {
        input.At(i, j, k, 0)      = static_cast<DataType>(counter);
        weights.At(0, i, j, k, 0) = static_cast<DataType>(counter);
        ++counter;
      }
    }
  }

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(input), std::make_shared<const TensorType>(weights)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(Convolution2DTest, saveparams_backward_3x3x3x2_5x3x3x3x2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = fetch::ml::ops::Convolution2D<TensorType>;
  using SPType     = typename OpType::SPType;

  SizeType const input_channels  = 3;
  SizeType const output_channels = 5;

  SizeType const input_width  = 3;
  SizeType const input_height = 3;

  SizeType const kernel_width  = 3;
  SizeType const kernel_height = 3;

  SizeType const output_width  = 1;
  SizeType const output_height = 1;

  SizeType const batch_size = 2;

  TensorType input({input_channels, input_height, input_width, batch_size});
  TensorType kernels({output_channels, input_channels, kernel_height, kernel_width, 1});
  TensorType error({output_channels, output_height, output_width, batch_size});

  // Generate input
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)
  {
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)
    {
      for (SizeType i_i{0}; i_i < input_height; ++i_i)
      {
        for (SizeType j_i{0}; j_i < input_width; ++j_i)
        {
          input(i_ic, i_i, j_i, i_b) = static_cast<DataType>(i_i + 1);
        }
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
          kernels(i_oc, i_ic, i_k, j_k, 0) = DataType{2};
        }
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
        for (SizeType j_o{0}; j_o < output_width; ++j_o)
        {
          error(i_oc, i_o, j_o, i_b) = static_cast<DataType>(i_o + 1);
        }
      }
    }
  }

  fetch::ml::ops::Convolution2D<TensorType> op;
  std::vector<TensorType>                   prediction = op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(input), std::make_shared<TensorType>(kernels)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  // test correct values
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
