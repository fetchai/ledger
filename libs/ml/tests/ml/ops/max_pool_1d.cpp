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

#include "ml/ops/max_pool_1d.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class MaxPool1DTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(MaxPool1DTest, MyTypes);

TYPED_TEST(MaxPool1DTest, forward_test_3_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType           data({1, 10});
  ArrayType           gt({1, 4});
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gtInput({3, 5, 7, 9});
  for (std::uint64_t i(0); i < 10; ++i)
  {
    data.Set(0, i, DataType(dataInput[i]));
  }

  for (std::uint64_t i(0); i < 4; ++i)
  {
    gt.Set(0, i, DataType(gtInput[i]));
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(3, 2);
  ArrayType                            prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(MaxPool1DTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType           data({1, 10});
  ArrayType           error({1, 10});
  ArrayType           gt({1, 10});
  std::vector<double> dataInput({1, -2, 3, -4, 10, -6, 7, -8, 9, -10});
  std::vector<double> errorInput({2, 2, 2, 2, 2, 2, 2, 2, 2, 2});
  std::vector<double> gtInput({0, 0, 2, 0, 4, 0, 0, 0, 2, 0});
  for (std::uint64_t i(0); i < 10; ++i)
  {
    data.Set(0, i, DataType(dataInput[i]));
    error.Set(0, i, DataType(errorInput[i]));
    gt.Set(0, i, DataType(gtInput[i]));
  }
  fetch::ml::ops::MaxPool1D<ArrayType> op(3, 2);
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType(1e-5), DataType(1e-5)));
}

TYPED_TEST(MaxPool1DTest, forward_test_4_2)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType           data({1, 10});
  ArrayType           gt({1, 4});
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gtInput({3, 5, 7, 9});
  for (std::uint64_t i(0); i < 10; ++i)
  {
    data.Set(0, i, DataType(dataInput[i]));
  }

  for (std::uint64_t i(0); i < 4; ++i)
  {
    gt.Set(0, i, DataType(gtInput[i]));
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(4, 2);
  ArrayType                            prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(MaxPool1DTest, forward_test_2_4)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType           data({1, 10});
  ArrayType           gt({1, 3});
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8, 9, -10});
  std::vector<double> gtInput({1, 5, 9});
  for (std::uint64_t i(0); i < 10; ++i)
  {
    data.Set(0, i, DataType(dataInput[i]));
  }

  for (std::uint64_t i(0); i < 3; ++i)
  {
    gt.Set(0, i, DataType(gtInput[i]));
  }

  fetch::ml::ops::MaxPool1D<ArrayType> op(2, 4);
  ArrayType                            prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}
