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

#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/ops/abs.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include <ml/ops/mask_fill.hpp>
#include <vector>

template <typename T>
class MaskFillTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                 fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

TYPED_TEST_CASE(MaskFillTest, MyTypes);

TYPED_TEST(MaskFillTest, forward_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType mask = ArrayType::FromString("1, 0, 1, 0, 0, 0, 0, 1, 1");
  mask.Reshape({3, 3, 1});

  ArrayType then_array = ArrayType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  then_array.Reshape({3, 3, 1});

  ArrayType gt = ArrayType::FromString("3, -100, 2, -100, -100, -100, -100, 1, -9");
  gt.Reshape({3, 3, 1});

  fetch::ml::ops::MaskFill<ArrayType> op(static_cast<DataType>(-100));

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(then_array)}));
  op.Forward(
      {std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(then_array)},
      prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaskFillTest, forward_test_mask_broadcasted)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType mask = ArrayType::FromString("1, 1, 0");
  mask.Reshape({1, 3, 1});

  ArrayType then_array = ArrayType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  then_array.Reshape({3, 3, 1});

  ArrayType gt = ArrayType::FromString("3, 6, 2, 1, 3, -2, -100, -100, -100");
  gt.Reshape({3, 3, 1});

  fetch::ml::ops::MaskFill<ArrayType> op(static_cast<DataType>(-100));

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(then_array)}));
  op.Forward(
      {std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(then_array)},
      prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaskFillTest, back_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType mask = ArrayType::FromString("1, 0, 1, 0, 0, 0, 0, 1, 1");
  mask.Reshape({3, 3, 1});

  ArrayType target_input = ArrayType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  target_input.Reshape({3, 3, 1});

  ArrayType error_signal = ArrayType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9");
  error_signal.Reshape({3, 3, 1});

  ArrayType gt_mask({3, 3, 1});

  ArrayType gt_then = ArrayType::FromString("1, 0, 3, 0, 0, 0, 0, 8, 9");
  gt_then.Reshape({3, 3, 1});

  fetch::ml::ops::MaskFill<ArrayType> op(static_cast<DataType>(-100));

  std::vector<TypeParam> prediction = op.Backward(
      {std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(target_input)},
      error_signal);

  // test correct values
  ASSERT_TRUE(prediction.at(0).AllClose(gt_mask, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction.at(1).AllClose(gt_then, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaskFillTest, back_test_broadcast_mask)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType mask = ArrayType::FromString("1, 1, 0");
  mask.Reshape({1, 3, 1});

  ArrayType target_input = ArrayType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  target_input.Reshape({3, 3, 1});

  ArrayType error_signal = ArrayType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9");
  error_signal.Reshape({3, 3, 1});

  ArrayType gt_mask({1, 3, 1});

  ArrayType gt_then = ArrayType::FromString("1, 2, 3, 4, 5, 6, 0, 0, 0");
  gt_then.Reshape({3, 3, 1});

  fetch::ml::ops::MaskFill<ArrayType> op(static_cast<DataType>(-100));

  std::vector<TypeParam> prediction = op.Backward(
      {std::make_shared<const ArrayType>(mask), std::make_shared<const ArrayType>(target_input)},
      error_signal);

  // test correct values
  ASSERT_TRUE(prediction.at(0).AllClose(gt_mask, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction.at(1).AllClose(gt_then, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
}
