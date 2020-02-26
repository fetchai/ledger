//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "test_types.hpp"

#include "math/base_types.hpp"
#include "ml/ops/divide.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

template <typename T>
class DivideTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(DivideTest, fetch::math::test::TensorFloatingTypes, );

TYPED_TEST(DivideTest, forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      " 8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt = TensorType::FromString(
      "0.125,	0.285714285714286,	0.5,	0.8,	1.25,	2,	3.5,	8;"
      "-0.125, 0.285714285714286,	-0.5,	0.8,	-1.25,	2,	-3.5,	8");

  fetch::ml::ops::Divide<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape({data_1.shape(), data_2.shape()}));
  op.Forward({std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(DivideTest, backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt_1 = TensorType::FromString(
      "0.125, 0.142857142857143, 0.333333333333333, 0.4, 0.75, 1, 2, 4;"
      "0.625, -0.714285714285714, -1, -1.2, -1.75, -2.33333333333333, -4, -8");

  TensorType gt_2 = TensorType::FromString(
      "-0.015625, -0.040816326530612, -0.166666666666667, -0.32, -0.9375, -2, -7, -32;"
      "-0.078125, 0.204081632653061, -0.5, 0.96, -2.1875, 4.66666666666667, -14, 64");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Divide<TensorType> op;
  std::vector<TensorType>            prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

}  // namespace
