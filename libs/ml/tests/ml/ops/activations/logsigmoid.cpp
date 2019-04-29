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

#include "ml/ops/activations/logsigmoid.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class LogSigmoidTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(LogSigmoidTest, MyTypes);

TYPED_TEST(LogSigmoidTest, forward_test)
{
  TypeParam           data(8);
  TypeParam           gt(8);
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gtInput(
      {-0.31326, -2.126928, -0.048587, -4.01815, -0.006715, -6.002476, -0.000911466, -8.000335});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
  }
  fetch::ml::ops::LogSigmoid<TypeParam> op;
  TypeParam                             prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(LogSigmoidTest, forward_3d_tensor_test)
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  TypeParam           data({2, 2, 2});
  TypeParam           gt({2, 2, 2});
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gtInput(
      {-0.31326, -2.126928, -0.048587, -4.01815, -0.006715, -6.002476, -0.000911466, -8.000335});

  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 2; ++j)
    {
      for (SizeType k(0); k < 2; ++k)
      {
        data.Set(i, j, k, DataType(dataInput[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, DataType(gtInput[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::LogSigmoid<TypeParam> op;
  TypeParam                             prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(LogSigmoidTest, backward_test)
{
  TypeParam           data(8);
  TypeParam           error(8);
  TypeParam           gt(8);
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double> gtInput({0, 0, 0, 0, 0.006692850, 0, 0, 0});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    error.Set(i, typename TypeParam::Type(errorInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
  }
  fetch::ml::ops::LogSigmoid<TypeParam> op;
  std::vector<TypeParam>                prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(
      prediction[0].AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(LogSigmoidTest, backward_3d_tensor_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 2, 2});
  ArrayType           error({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double> gtInput({0, 0, 0, 0, 0.006692850, 0, 0, 0});

  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 2; ++j)
    {
      for (SizeType k(0); k < 2; ++k)
      {
        data.Set(i, j, k, DataType(dataInput[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, DataType(errorInput[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, DataType(gtInput[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::LogSigmoid<ArrayType> op;
  std::vector<ArrayType>                prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType(1e-5), DataType(1e-5)));
}