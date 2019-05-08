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

#include "ml/ops/activations/dropout.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class DropoutTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(DropoutTest, MyTypes);

TYPED_TEST(DropoutTest, forward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  ArrayType gt   = ArrayType::FromString("0, -2, 0,  0, 5, -6, 7, -8");

  fetch::ml::ops::Dropout<ArrayType> op(DataType{0.5}, 12345);
  ArrayType                          prediction = op.fetch::ml::template Ops<ArrayType>::Forward(
      std::vector<std::reference_wrapper<ArrayType const>>({data}));

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test after generating new random alpha value
  gt = ArrayType::FromString("1, -2, 3, 0, 5, 0, 7, 0");

  prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test with is_training set to false
  op.SetTraining(false);

  gt = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(DropoutTest, forward_3d_tensor_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({0, -2, 0, 0, 5, -6, 7, -8});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, static_cast<DataType>(data_input[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, static_cast<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::Dropout<ArrayType> op(DataType{0.5f}, 12345);
  TypeParam                          prediction = op.fetch::ml::template Ops<TypeParam>::Forward(
      std::vector<std::reference_wrapper<TypeParam const>>({data}));

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(DropoutTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  ArrayType error = ArrayType::FromString("0, 0, 0, 0, 1, 1, 0, 0");
  ArrayType gt    = ArrayType::FromString("0, 0, 0, 0, 1, 1, 0, 0");

  fetch::ml::ops::Dropout<ArrayType> op(DataType{0.5f}, 12345);

  // It's necessary to do Forward pass first
  op.fetch::ml::template Ops<ArrayType>::Forward(
      std::vector<std::reference_wrapper<ArrayType const>>({data}));

  std::vector<ArrayType> prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test after generating new random alpha value
  // Forward pass will update random value
  op.fetch::ml::template Ops<ArrayType>::Forward(
      std::vector<std::reference_wrapper<ArrayType const>>({data}));

  gt         = ArrayType::FromString("0, 0, 0, 0, 1, 0, 0, 0");
  prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test with is_training set to false
  op.SetTraining(false);

  gt         = ArrayType::FromString("0, 0, 0, 0, 1, 1, 0, 0");
  prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(DropoutTest, backward_3d_tensor_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 2, 2});
  ArrayType           error({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> error_input({0, 0, 0, 0, 1, 1, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 1, 1, 0, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, static_cast<DataType>(data_input[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, static_cast<DataType>(error_input[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, static_cast<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::Dropout<ArrayType> op(DataType{0.5f}, 12345);

  // It's necessary to do Forward pass first
  op.fetch::ml::template Ops<ArrayType>::Forward(
      std::vector<std::reference_wrapper<ArrayType const>>({data}));

  std::vector<ArrayType> prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}
