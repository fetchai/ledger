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
#include "ml/ops/max_pool_1d.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class MaxPool1DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(MaxPool1DTest, MyTypes);

TYPED_TEST(MaxPool1DTest, forward_test_3_2_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({1, 10, 2});
  ArrayType           gt({1, 4, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 7, 9});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) = static_cast<DataType>(data_input[i]) + static_cast<DataType>(i_b * 10);
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      gt(0, i, i_b) = static_cast<DataType>(gt_input[i]) + static_cast<DataType>(i_b * 10);
    }
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(3, 2);

  ArrayType prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool1DTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({1, 10, 2});
  ArrayType           error({1, 4, 2});
  ArrayType           gt({1, 10, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});
  std::vector<double> gt_input1({0, 0, 2, 0, 7, 0, 0, 0, 5, 0});
  std::vector<double> gt_input2({0, 0, 3, 0, 9, 0, 0, 0, 6, 0});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) = static_cast<DataType>(data_input[i]) + static_cast<DataType>(i_b);
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      error(0, i, i_b) = static_cast<DataType>(errorInput[i]) + static_cast<DataType>(i_b);
    }
  }

  for (SizeType i{0}; i < 10; ++i)
  {
    gt(0, i, 0) = static_cast<DataType>(gt_input1[i]);
    gt(0, i, 1) = static_cast<DataType>(gt_input2[i]);
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(3, 2);
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool1DTest, backward_test_2_channels)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 5, 2});
  ArrayType           error({2, 2, 2});
  ArrayType           gt({2, 5, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});
  std::vector<double> gt_input({0, 0, 2, 0, 3, 0, 0, 0, 9, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = static_cast<DataType>(data_input[i * 5 + j]);
      gt(i, j, 0)   = static_cast<DataType>(gt_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = static_cast<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(4, 1);
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool1DTest, forward_test_4_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({1, 10, 1});
  ArrayType           gt({1, 4, 1});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 7, 9});
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = static_cast<DataType>(data_input[i]);
  }

  for (SizeType i{0}; i < 4; ++i)
  {
    gt(0, i, 0) = static_cast<DataType>(gt_input[i]);
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(4, 2);

  ArrayType prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool1DTest, forward_test_2_channels_4_1_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 5, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 9, 9});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) =
            static_cast<DataType>(data_input[i * 5 + j]) + static_cast<DataType>(i_b * 10);
      }
    }

    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 2; ++j)
      {
        gt(i, j, i_b) =
            static_cast<DataType>(gt_input[i * 2 + j]) + static_cast<DataType>(i_b * 10);
      }
    }
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(4, 1);

  ArrayType prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(MaxPool1DTest, forward_test_2_4_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({1, 10, 2});
  ArrayType           gt({1, 3, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({1, 5, 9});
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = static_cast<DataType>(data_input[i]);
  }

  for (SizeType i{0}; i < 3; ++i)
  {
    gt(0, i, 0) = static_cast<DataType>(gt_input[i]);
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(2, 4);

  ArrayType prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}
