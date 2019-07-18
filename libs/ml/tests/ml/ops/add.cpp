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

#include "ml/ops/add.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class AddTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                 fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

TYPED_TEST_CASE(AddTest, MyTypes);

TYPED_TEST(AddTest, forward_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType data_1 = ArrayType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  ArrayType data_2 = ArrayType::FromString(
      "8;"
      "-8");

  ArrayType gt = ArrayType::FromString(
      "9,  6, 11,  4, 13,  2, 15, 0;"
      "-7, -6, -5, -4, -3,	-2,	-1,	0");

  fetch::ml::ops::Add<ArrayType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<ArrayType>(data_1), std::make_shared<ArrayType>(data_2)}));
  op.Forward({std::make_shared<ArrayType>(data_1), std::make_shared<ArrayType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AddTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data_1 = ArrayType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  ArrayType data_2 = ArrayType::FromString(
      "8;"
      "-8");

  ArrayType gt_1 = ArrayType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  ArrayType gt_2 = ArrayType::FromString(
      "8;"
      "16");

  ArrayType error = ArrayType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  fetch::ml::ops::Add<ArrayType> op;
  std::vector<ArrayType>         prediction = op.Backward(
      {std::make_shared<ArrayType>(data_1), std::make_shared<ArrayType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}
