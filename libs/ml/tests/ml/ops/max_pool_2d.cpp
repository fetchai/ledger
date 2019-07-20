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
#include "ml/ops/max_pool_2d.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <vector>

template <typename T>
class MaxPool2DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(MaxPool2DTest, MyTypes);

TYPED_TEST(MaxPool2DTest, forward_test_3_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_width  = 10;
  SizeType const input_height = 5;

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  ArrayType           data({1, input_width, input_height, batch_size});
  ArrayType           gt({1, output_width, output_height, batch_size});
  std::vector<double> gt_input({4, 8, 12, 16, 8, 16, 24, 32});
  for (SizeType i{0}; i < input_width; ++i)
  {
    for (SizeType j{0}; j < input_height; ++j)
    {
      data(0, i, j, 0) = static_cast<DataType>(i * j);
    }
  }

  for (SizeType i{0}; i < output_width; ++i)
  {
    for (SizeType j{0}; j < output_height; ++j)
    {
      gt(0, i, j, 0) = static_cast<DataType>(gt_input[i + j * output_width]);
    }
  }

  fetch::ml::ops::MaxPool2D<ArrayType> op(3, 2);

  ArrayType prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool2DTest, forward_2_channels_test_3_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 10;
  SizeType const input_height  = 5;

  SizeType const output_width  = 4;
  SizeType const output_height = 2;

  SizeType const batch_size = 2;

  ArrayType           data({channels_size, input_width, input_height, batch_size});
  ArrayType           gt({channels_size, output_width, output_height, batch_size});
  std::vector<double> gt_input({4, 8, 12, 16, 8, 16, 24, 32, 8, 16, 24, 32, 16, 32, 48, 64});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = static_cast<DataType>((c + 1) * i * j);
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        gt(c, i, j, 0) = static_cast<DataType>(
            gt_input[c * output_width * output_height + (i + j * output_width)]);
      }
    }
  }

  fetch::ml::ops::MaxPool2D<ArrayType> op(3, 2);
  ArrayType                            prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool2DTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  ArrayType data({1, input_width, input_height, batch_size});
  ArrayType error({1, output_width, output_height, batch_size});
  ArrayType gt({1, input_width, input_height, batch_size});

  for (SizeType i{0}; i < input_width; ++i)
  {
    for (SizeType j{0}; j < input_height; ++j)
    {
      data(0, i, j, 0) = static_cast<DataType>(i * j);
      gt(0, i, j, 0)   = DataType{0};
    }
  }

  for (SizeType i{0}; i < output_width; ++i)
  {
    for (SizeType j{0}; j < output_height; ++j)
    {
      error(0, i, j, 0) = static_cast<DataType>(1 + i + j);
    }
  }

  gt(0, 2, 2, 0) = DataType{1};
  gt(0, 4, 2, 0) = DataType{2};
  gt(0, 2, 4, 0) = DataType{2};
  gt(0, 4, 4, 0) = DataType{3};

  fetch::ml::ops::MaxPool2D<ArrayType> op(3, 2);
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool2DTest, backward_2_channels_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const channels_size = 2;
  SizeType const input_width   = 5;
  SizeType const input_height  = 5;
  SizeType const output_width  = 2;
  SizeType const output_height = 2;
  SizeType const batch_size    = 2;

  ArrayType data({channels_size, input_width, input_height, batch_size});
  ArrayType error({channels_size, output_width, output_height, batch_size});
  ArrayType gt({channels_size, input_width, input_height, batch_size});

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < input_width; ++i)
    {
      for (SizeType j{0}; j < input_height; ++j)
      {
        data(c, i, j, 0) = static_cast<DataType>((c + 1) * i * j);
        gt(c, i, j, 0)   = DataType{0};
      }
    }
  }

  for (SizeType c{0}; c < channels_size; ++c)
  {
    for (SizeType i{0}; i < output_width; ++i)
    {
      for (SizeType j{0}; j < output_height; ++j)
      {
        error(c, i, j, 0) = static_cast<DataType>((c + 1) * (1 + i + j));
      }
    }
  }

  gt(0, 2, 2, 0) = DataType{1};
  gt(0, 4, 2, 0) = DataType{2};
  gt(0, 2, 4, 0) = DataType{2};
  gt(0, 4, 4, 0) = DataType{3};
  gt(1, 2, 2, 0) = DataType{2};
  gt(1, 4, 2, 0) = DataType{4};
  gt(1, 2, 4, 0) = DataType{4};
  gt(1, 4, 4, 0) = DataType{6};

  fetch::ml::ops::MaxPool2D<ArrayType> op(3, 2);
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}
