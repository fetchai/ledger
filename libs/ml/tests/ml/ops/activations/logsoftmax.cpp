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

#include "math/fixed_point/fixed_point.hpp"
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
  using DataType      = typename TypeParam::Type;
  using ArrayType     = TypeParam;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;

  ArrayType data = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  ArrayType gt   = ArrayType::FromString(
      "-6.14520134, -9.14520134, -4.14520134, -11.14520134, -2.14520134, -13.14520134, "
      "-0.14520134, -15.14520134");

  fetch::ml::ops::LogSoftmax<ArrayType> op;
  ArrayType                             prediction(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-3f}, DataType{1e-3f}));
}

TYPED_TEST(LogSoftmaxTest, forward_2d_tensor_axis_0_test)
{
  using DataType      = typename TypeParam::Type;
  using ArrayType     = TypeParam;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;

  ArrayType           data({3, 3});
  ArrayType           gt({3, 3});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> gt_input({-2.1328e+00, -5.1328e+00, -1.3285e-01, -9.0001e+00, -1.4008e-04,
                                -1.1000e+01, -2.1269e+00, -1.7127e+01, -1.2693e-01});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      data.Set(i, j, static_cast<DataType>(data_input[j + 3 * i]));
      gt.Set(i, j, static_cast<DataType>(gt_input[j + 3 * i]));
    }
  }

  fetch::ml::ops::LogSoftmax<ArrayType> op{0};
  ArrayType                             prediction(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-3f}, DataType{1e-3f}));
}

TYPED_TEST(LogSoftmaxTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  ArrayType error = ArrayType::FromString("0, 0, 0, 1, 1, 1, 0, 0");
  ArrayType gt    = ArrayType::FromString(
      "-6.4312e-03, -3.2019e-04, -4.7521e-02,  9.9996e-01,  6.4887e-01, 9.9999e-01, -2.59454, "
      "-7.9368e-07");

  fetch::ml::ops::LogSoftmax<ArrayType> op;
  std::vector<ArrayType>                prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(LogSoftmaxTest, backward_2d_tensor_axis_0_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({3, 3});
  ArrayType           error({3, 3});
  ArrayType           gt({3, 3});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> errorInput({0.1, 0, 0, 0, 0.5, 0, 0, 0, 0.9});
  std::vector<double> gt_input({8.8150e-02, -5.8998e-04, -8.7560e-02, -6.1696e-05, 7.0026e-05,
                                -8.3497e-06, -1.0728e-01, -3.2818e-08, 1.0728e-01});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      data.Set(i, j, static_cast<DataType>(data_input[j + 3 * i]));
      error.Set(i, j, static_cast<DataType>(errorInput[j + 3 * i]));
      gt.Set(i, j, static_cast<DataType>(gt_input[j + 3 * i]));
    }
  }
  fetch::ml::ops::LogSoftmax<ArrayType> op{0};
  std::vector<ArrayType>                prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}
