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

#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include "ml/ops/activation.hpp"
#include <gtest/gtest.h>

template <typename T>
class LogSoftmaxTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(LogSoftmaxTest, MyTypes);

TYPED_TEST(LogSoftmaxTest, forward_test)
{
  TypeParam           data(8);
  TypeParam           gt(8);
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gtInput({-6.14520134, -9.14520134, -4.14520134, -11.14520134, -2.14520134,
                               -13.14520134, -0.14520134, -15.14520134});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
  }
  fetch::ml::ops::LogSoftmax<TypeParam> op;
  TypeParam                             prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-2), typename TypeParam::Type(1e-2)));
}

TYPED_TEST(LogSoftmaxTest, forward_2d_tensor_axis_0_test)
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  TypeParam           data({3, 3});
  TypeParam           gt({3, 3});
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> gtInput({-6.00249, -7.00091, -6.00248, -11.00249, -0.00091, -15.00248,
                               -0.00249, -13.00091, -0.00248});
  for (SizeType i(0); i < 3; ++i)
  {
    for (SizeType j(0); j < 3; ++j)
    {
      data.Set(i, j, DataType(dataInput[i + 3 * j]));
      gt.Set(i, j, DataType(gtInput[i + 3 * j]));
    }
  }

  fetch::ml::ops::LogSoftmax<TypeParam> op(0);
  TypeParam                             prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-2), typename TypeParam::Type(1e-2)));
}

TYPED_TEST(LogSoftmaxTest, backward_test)
{
  using DataType = typename TypeParam::Type;

  TypeParam           data(8);
  TypeParam           error(8);
  TypeParam           gt(8);
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double> gtInput({0.5, 0.5, 0.5, 0.5, 0.268941, 0.5, 0.5, 0.5});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data.Set(i, DataType(dataInput[i]));
    error.Set(i, DataType(errorInput[i]));
    gt.Set(i, DataType(gtInput[i]));
  }
  fetch::ml::ops::LogSoftmax<TypeParam> op;
  std::vector<TypeParam>                prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(
      prediction[0].AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(LogSoftmaxTest, backward_2d_tensor_axis_0_test)
{
  //    using DataType  = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  TypeParam           data({3, 3});
  TypeParam           error({3, 3});
  TypeParam           gt({3, 3});
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0, 0});
  std::vector<double> gtInput({0.5, 0.5, 0.5, 0.5, 0.268941, 0.5, 0.5, 0.5, 0.5});
  for (SizeType i(0); i < 3; ++i)
  {
    for (SizeType j(0); j < 3; ++j)
    {
      data.Set(i, j, typename TypeParam::Type(dataInput[i + 3 * j]));
      error.Set(i, j, typename TypeParam::Type(errorInput[i + 3 * j]));
      gt.Set(i, j, typename TypeParam::Type(gtInput[i + 3 * j]));
    }
  }
  fetch::ml::ops::LogSoftmax<TypeParam> op(0);
  std::vector<TypeParam>                prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(
      prediction[0].AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}