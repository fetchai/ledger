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

#include "math/base_types.hpp"
#include "ml/ops/maximum.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class MaximumTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MaximumTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(MaximumTest, forward_test)
{
  using TensorType  = TypeParam;
  using DataType    = typename TypeParam::Type;
  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt = TensorType::FromString(
      "8,	-2, 6, -4, 5, -3, 7, -1;"
      "1,  7, 3,  5, 5,  6, 7,  8");

  fetch::ml::ops::Maximum<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}));
  op.Forward({std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MaximumTest, backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt_1 = TensorType::FromString(
      "0, -1, 0, -2, 3, 0, 4, 0;"
      "5, 0, 6, 0, 7, -7, 8, -8");

  TensorType gt_2 = TensorType::FromString(
      "1, 0, 2, 0, 0, -3, 0, -4;"
      "0, -5, 0, -6, 0, 0, 0, 0");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Maximum<TensorType> op;
  std::vector<TensorType>             prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

}  // namespace
