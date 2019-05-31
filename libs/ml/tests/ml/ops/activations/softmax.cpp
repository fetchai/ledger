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
class SoftmaxTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(SoftmaxTest, MyTypes);

TYPED_TEST(SoftmaxTest, forward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  ArrayType gt   = ArrayType::FromString(
      "2.1437e-03, 1.0673e-04, 1.5840e-02, 1.4444e-05, 1.1704e-01, 1.9548e-06, 8.6485e-01, "
      "2.6456e-07");

  fetch::ml::ops::Softmax<ArrayType> op;
  ArrayType                          prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(SoftmaxTest, forward_2d_tensor_axis_0_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({3, 3});
  ArrayType           gt({3, 3});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> gt_input({1.1850e-01, 5.8998e-03, 8.7560e-01, 1.2339e-04, 9.9986e-01,
                                1.6699e-05, 1.1920e-01, 3.6464e-08, 8.8080e-01});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      data.Set(i, j, static_cast<DataType>(data_input[j + 3 * i]));
      gt.Set(i, j, static_cast<DataType>(gt_input[j + 3 * i]));
    }
  }

  fetch::ml::ops::Softmax<ArrayType> op{0};
  ArrayType                          prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, static_cast<DataType>(1e-4), static_cast<DataType>(1e-4)));
}

TYPED_TEST(SoftmaxTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  ArrayType error = ArrayType::FromString("0, 0, 0, 0, 1, 0, 0, 0");
  ArrayType gt    = ArrayType::FromString(
      "-2.5091e-04, -1.2492e-05, -1.8540e-03, -1.6906e-06, 1.0335e-01, -2.2880e-07, -1.0123e-01, "
      "-3.0965e-08");

  fetch::ml::ops::Softmax<ArrayType> op;
  std::vector<ArrayType>             prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(SoftmaxTest, backward_2d_tensor_axis_0_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({3, 3});
  ArrayType           error({3, 3});
  ArrayType           gt({3, 3});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0, 0});
  std::vector<double> gt_input({0, 0, 0, -1.2338e-04, 1.4005e-04, -1.6697e-05, 0, 0, 0});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      data.Set(i, j, static_cast<DataType>(data_input[j + 3 * i]));
      error.Set(i, j, static_cast<DataType>(errorInput[j + 3 * i]));
      gt.Set(i, j, static_cast<DataType>(gt_input[j + 3 * i]));
    }
  }

  fetch::ml::ops::Softmax<ArrayType> op{0};
  std::vector<ArrayType>             prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}
