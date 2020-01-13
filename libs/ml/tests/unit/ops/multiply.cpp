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
#include "ml/ops/multiply.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class MultiplyTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MultiplyTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(MultiplyTest, forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt = TensorType::FromString(
      "8, 14, 18,20, 20,18, 14,8;"
      "-8,  14,-18, 20,-20, 18,-14, 8");

  fetch::ml::ops::Multiply<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}));
  op.Forward({std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  ASSERT_TRUE(!fetch::math::state_overflow<typename TypeParam::Type>());
}

TYPED_TEST(MultiplyTest, backward_test_NMB_N11)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");
  data_1.Reshape({2, 2, 2});

  TensorType data_2 = TensorType::FromString("1, -1");
  data_2.Reshape({2, 1, 1});

  TensorType error = TensorType::FromString(
      "0, 1, 4, 5;"
      "2, 3, 6, 7");
  error.Reshape({2, 2, 2});

  TensorType gt_1 = TensorType::FromString(
      "0, 1, 4, 5;"
      "-2, -3, -6, -7");
  gt_1.Reshape({2, 2, 2});

  TensorType gt_2 = TensorType::FromString(
      "52;"
      "116");
  gt_2.Reshape({2, 1, 1});

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values and shape
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[0].shape() == data_1.shape());
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
}

TYPED_TEST(MultiplyTest, backward_test_NMB_111)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");
  data_1.Reshape({2, 2, 2});

  TensorType data_2 = TensorType::FromString("-1");
  data_2.Reshape({1, 1, 1});

  TensorType error = TensorType::FromString(
      "0, 1, 4, 5;"
      "2, 3, 6, 7");
  error.Reshape({2, 2, 2});

  TensorType gt_1 = TensorType::FromString(
      "0, -1, -4, -5;"
      "-2, -3, -6, -7");
  gt_1.Reshape({2, 2, 2});

  TensorType gt_2 = TensorType::FromString("168");
  gt_2.Reshape({1, 1, 1});

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values and shape
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[0].shape() == data_1.shape());
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
}

TYPED_TEST(MultiplyTest, backward_test_NB_N1)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");

  TensorType data_2 = TensorType::FromString("1, -1");
  data_2.Reshape({2, 1});

  TensorType error = TensorType::FromString(
      "0, 1, 4, 5;"
      "2, 3, 6, 7");

  TensorType gt_1 = TensorType::FromString(
      "0, 1, 4, 5;"
      "-2, -3, -6, -7");

  TensorType gt_2 = TensorType::FromString(
      "52;"
      "116");
  gt_2.Reshape({2, 1});

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values and shape
  ASSERT_TRUE(prediction[0].shape() == data_1.shape());
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MultiplyTest, backward_test_NB_NB)
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
      "8,	   7,  12,  10,  12,   9,   8,  4;"
      "-40, -35, -36, -30,	-28, -21, -16, -8");

  TensorType gt_2 = TensorType::FromString(
      "1,   2,	 6,	  8, 15,  18, 28,  32;"
      "5, -10, 18,	-24, 35, -42, 56, -64");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

}  // namespace
