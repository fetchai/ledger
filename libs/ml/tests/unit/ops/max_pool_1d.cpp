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
#include "ml/ops/max_pool_1d.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using namespace fetch::ml;

template <typename T>
class MaxPool1DTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MaxPool1DTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(MaxPool1DTest, forward_test_3_2_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({1, 10, 2});
  TensorType          gt({1, 4, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 7, 9});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) =
          fetch::math::AsType<DataType>(data_input[i]) + fetch::math::AsType<DataType>(i_b * 10);
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      gt(0, i, i_b) =
          fetch::math::AsType<DataType>(gt_input[i]) + fetch::math::AsType<DataType>(i_b * 10);
    }
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(3, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaxPool1DTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({1, 10, 2});
  TensorType          error({1, 4, 2});
  TensorType          gt({1, 10, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});
  std::vector<double> gt_input1({0, 0, 2, 0, 7, 0, 0, 0, 5, 0});
  std::vector<double> gt_input2({0, 0, 3, 0, 9, 0, 0, 0, 6, 0});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 10; ++i)
    {
      data(0, i, i_b) =
          fetch::math::AsType<DataType>(data_input[i]) + fetch::math::AsType<DataType>(i_b);
    }
    for (SizeType i{0}; i < 4; ++i)
    {
      error(0, i, i_b) =
          fetch::math::AsType<DataType>(errorInput[i]) + fetch::math::AsType<DataType>(i_b);
    }
  }

  for (SizeType i{0}; i < 10; ++i)
  {
    gt(0, i, 0) = fetch::math::AsType<DataType>(gt_input1[i]);
    gt(0, i, 1) = fetch::math::AsType<DataType>(gt_input2[i]);
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(3, 2);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaxPool1DTest, backward_test_2_channels)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 5, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 5, 2});
  std::vector<double> data_input({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 3, 4, 5});
  std::vector<double> gt_input({0, 0, 2, 0, 3, 0, 0, 0, 9, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      data(i, j, 0) = fetch::math::AsType<DataType>(data_input[i * 5 + j]);
      gt(i, j, 0)   = fetch::math::AsType<DataType>(gt_input[i * 5 + j]);
    }
  }

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      error(i, j, 0) = fetch::math::AsType<DataType>(errorInput[i * 2 + j]);
    }
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(4, 1);
  std::vector<TensorType>               prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaxPool1DTest, forward_test_4_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({1, 10, 1});
  TensorType          gt({1, 4, 1});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 7, 9});
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = fetch::math::AsType<DataType>(data_input[i]);
  }

  for (SizeType i{0}; i < 4; ++i)
  {
    gt(0, i, 0) = fetch::math::AsType<DataType>(gt_input[i]);
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(4, 2);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaxPool1DTest, forward_test_2_channels_4_1_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 5, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({3, 5, 9, 9});

  for (SizeType i_b{0}; i_b < 2; i_b++)
  {
    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 5; ++j)
      {
        data(i, j, i_b) = fetch::math::AsType<DataType>(data_input[i * 5 + j]) +
                          fetch::math::AsType<DataType>(i_b * 10);
      }
    }

    for (SizeType i{0}; i < 2; ++i)
    {
      for (SizeType j{0}; j < 2; ++j)
      {
        gt(i, j, i_b) = fetch::math::AsType<DataType>(gt_input[i * 2 + j]) +
                        fetch::math::AsType<DataType>(i_b * 10);
      }
    }
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(4, 1);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaxPool1DTest, forward_test_2_4_2)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({1, 10, 2});
  TensorType          gt({1, 3, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gt_input({1, 5, 9});
  for (SizeType i{0}; i < 10; ++i)
  {
    data(0, i, 0) = fetch::math::AsType<DataType>(data_input[i]);
  }

  for (SizeType i{0}; i < 3; ++i)
  {
    gt(0, i, 0) = fetch::math::AsType<DataType>(gt_input[i]);
  }

  fetch::ml::ops::MaxPool1D<TensorType> op(2, 4);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

}  // namespace
