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

#include "ml/ops/activations/randomized_relu.hpp"

#include "math/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"

#include <gtest/gtest.h>

template <typename T>
class RandomizedReluTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(RandomizedReluTest, MyTypes);

TYPED_TEST(RandomizedReluTest, forward_test)
{
  using DataType      = typename TypeParam::Type;
  using ArrayType     = TypeParam;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;

  ArrayType           data(8);
  ArrayType           gt(8);
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({1, -0.062793536, 3, -0.12558707, 5, -0.1883806, 7, -0.2511741});
  for (SizeType i{0}; i < 8; ++i)
  {
    data.Set(i, static_cast<DataType>(data_input[i]));
    gt.Set(i, static_cast<DataType>(gt_input[i]));
  }
  fetch::ml::ops::RandomizedRelu<ArrayType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  ArrayType                                 prediction(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test after generating new random alpha value
  gt_input = {1, -0.157690314, 3, -0.315380628, 5, -0.47307094, 7, -0.63076125644};
  for (SizeType i{0}; i < 8; ++i)
  {
    gt.Set(i, static_cast<DataType>(gt_input[i]));
  }

  prediction = ArrayType(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test with is_training set to false
  op.SetTraining(false);

  gt_input = {1, -0.11, 3, -0.22, 5, -0.33, 7, -0.44};
  for (SizeType i{0}; i < 8; ++i)
  {
    gt.Set(i, static_cast<DataType>(gt_input[i]));
  }

  prediction = ArrayType(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(RandomizedReluTest, forward_3d_tensor_test)
{
  using DataType      = typename TypeParam::Type;
  using ArrayType     = TypeParam;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;

  ArrayType           data({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({1, -0.062793536, 3, -0.12558707, 5, -0.1883806, 7, -0.2511741});

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

  fetch::ml::ops::RandomizedRelu<ArrayType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  ArrayType                                 prediction(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(RandomizedReluTest, backward_test)
{
  using DataType      = typename TypeParam::Type;
  using ArrayType     = TypeParam;
  using SizeType      = typename TypeParam::SizeType;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;

  ArrayType           data(8);
  ArrayType           error(8);
  ArrayType           gt(8);
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 1, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 1, 0.079588953, 0, 0});
  for (SizeType i{0}; i < 8; ++i)
  {
    data.Set(i, static_cast<DataType>(data_input[i]));
    error.Set(i, static_cast<DataType>(errorInput[i]));
    gt.Set(i, static_cast<DataType>(gt_input[i]));
  }
  fetch::ml::ops::RandomizedRelu<ArrayType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  std::vector<ArrayType>                    prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test after generating new random alpha value
  // Forward pass will update random value
  ArrayType output(op.ComputeOutputShape({data}));
  op.Forward(VecTensorType({data}), output);

  gt_input = {0, 0, 0, 0, 1, 0.0788452, 0, 0};
  for (SizeType i{0}; i < 8; ++i)
  {
    gt.Set(i, static_cast<DataType>(gt_input[i]));
  }
  prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));

  // Test with is_training set to false
  op.SetTraining(false);

  gt_input = {0, 0, 0, 0, 1, 0.055, 0, 0};
  for (SizeType i{0}; i < 8; ++i)
  {
    gt.Set(i, static_cast<DataType>(gt_input[i]));
  }
  prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(RandomizedReluTest, backward_3d_tensor_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType           data({2, 2, 2});
  ArrayType           error({2, 2, 2});
  ArrayType           gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 1, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 1, 0.079588953, 0, 0});

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

  fetch::ml::ops::RandomizedRelu<ArrayType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  std::vector<ArrayType>                    prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType{1e-5f}, DataType{1e-5f}));
}
