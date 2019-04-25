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

#include "ml/ops/max_pool_2d.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

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

  ArrayType           data({1, input_width, input_height});
  ArrayType           gt({1, output_width, output_height});
  std::vector<double> gtInput({4, 8, 12, 16, 8, 16, 24, 32});
  for (std::uint64_t i(0); i < input_width; ++i)
  {
    for (std::uint64_t j(0); j < input_height; ++j)
    {
      data.Set(0, i, j, DataType(i * j));
    }
  }

  for (std::uint64_t i(0); i < output_width; ++i)
  {
    for (std::uint64_t j(0); j < output_height; ++j)
    {

      data.Set(0, i, j, DataType(i * j));
      gt.Set(0, i, j, DataType(gtInput[i + j * output_width]));
    }
  }

  fetch::ml::ops::MaxPool2D<ArrayType> op(3, 2);
  ArrayType                            prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(MaxPool2DTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType const input_width  = 5;
  SizeType const input_height = 5;

  ArrayType data({1, input_width, input_height});
  ArrayType error({1, input_width, input_height});
  ArrayType gt({1, input_width, input_height});

  for (std::uint64_t i(0); i < input_width; ++i)
  {
    for (std::uint64_t j(0); j < input_height; ++j)
    {
      data.Set(0, i, j, DataType(i * j));
      error.Set(0, i, j, DataType(2));
      gt.Set(0, i, j, DataType(0));
    }
  }

  gt.Set(0, 2, 2, DataType(2));
  gt.Set(0, 4, 2, DataType(2));
  gt.Set(0, 2, 4, DataType(2));
  gt.Set(0, 4, 4, DataType(2));

  fetch::ml::ops::MaxPool2D<ArrayType> op(3, 2);
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType(1e-5), DataType(1e-5)));
}
