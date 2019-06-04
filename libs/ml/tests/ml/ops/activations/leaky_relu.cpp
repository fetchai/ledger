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

#include "ml/ops/activations/leaky_relu.hpp"
#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include <gtest/gtest.h>

template <typename T>
class LeakyReluTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(LeakyReluTest, MyTypes);

TYPED_TEST(LeakyReluTest, forward_test)
{
  using DataType      = typename TypeParam::Type;
  using ArrayType     = TypeParam;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;

  ArrayType data = ArrayType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  ArrayType gt   = ArrayType::FromString(R"(1, -0.02, 3, -0.04, 5, -0.06, 7, -0.08)");

  fetch::ml::ops::LeakyRelu<ArrayType> op(DataType{0.01f});
  ArrayType                            prediction(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(LeakyReluTest, forward_3d_tensor_test)
{
  using DataType      = typename TypeParam::Type;
  using ArrayType     = TypeParam;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;

  ArrayType           data({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({1, -0.02, 3, -0.04, 5, -0.06, 7, -0.08});

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

  fetch::ml::ops::LeakyRelu<ArrayType> op(DataType{0.01f});

  ArrayType prediction(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(LeakyReluTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString(R"(1, -2, 3, -4, 5, -6, 7, -8)");
  ArrayType error = ArrayType::FromString(R"(0, 0, 0, 0.5, 1, 1, 0, 0)");
  ArrayType gt    = ArrayType::FromString(R"(0, 0, 0, 0.005, 1, 0.01, 0, 0)");

  fetch::ml::ops::LeakyRelu<ArrayType> op(DataType{0.01f});
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(LeakyReluTest, backward_3d_tensor_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 2, 2});
  ArrayType           error({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0.5, 1, 1, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0.005, 1, 0.01, 0, 0});

  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 2; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        data.Set(i, j, k, static_cast<DataType>(data_input[i + 2 * (j + 2 * k)]));
        error.Set(i, j, k, static_cast<DataType>(errorInput[i + 2 * (j + 2 * k)]));
        gt.Set(i, j, k, static_cast<DataType>(gt_input[i + 2 * (j + 2 * k)]));
      }
    }
  }

  fetch::ml::ops::LeakyRelu<ArrayType> op(DataType{0.01f});
  std::vector<ArrayType>               prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}