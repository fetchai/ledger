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
#include "ml/ops/switch.hpp"

#include "gtest/gtest.h"

#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class SwitchTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(SwitchTest, fetch::math::test::TensorFloatingTypes, );

TYPED_TEST(SwitchTest, forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType mask = TensorType::FromString("1, 0, 1, 0, 0, 0, 0, 1, 1");
  mask.Reshape({3, 3, 1});

  TensorType then_array = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  then_array.Reshape({3, 3, 1});

  TensorType else_array({3, 3, 1});
  else_array.Fill(static_cast<DataType>(-100));

  TensorType gt = TensorType::FromString("3, -100, 2, -100, -100, -100, -100, 1, -9");
  gt.Reshape({3, 3, 1});

  fetch::ml::ops::Switch<TensorType> op;

  TypeParam prediction(
      op.ComputeOutputShape({mask.shape(), then_array.shape(), else_array.shape()}));
  op.Forward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(then_array),
       std::make_shared<const TensorType>(else_array)},
      prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SwitchTest, forward_test_mask_broadcasted)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType mask = TensorType::FromString("1, 1, 0");
  mask.Reshape({1, 3, 1});

  TensorType then_array = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  then_array.Reshape({3, 3, 1});

  TensorType else_array({3, 3, 1});
  else_array.Fill(static_cast<DataType>(-100));

  TensorType gt = TensorType::FromString("3, 6, 2, 1, 3, -2, -100, -100, -100");
  gt.Reshape({3, 3, 1});

  fetch::ml::ops::Switch<TensorType> op;

  TypeParam prediction(
      op.ComputeOutputShape({mask.shape(), then_array.shape(), else_array.shape()}));
  op.Forward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(then_array),
       std::make_shared<const TensorType>(else_array)},
      prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SwitchTest, backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType mask = TensorType::FromString("1, 0, 1, 0, 0, 0, 0, 1, 1");
  mask.Reshape({3, 3, 1});

  TensorType target_input = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  target_input.Reshape({3, 3, 1});

  TensorType mask_value({3, 3, 1});
  mask_value.Fill(static_cast<DataType>(-100));

  TensorType error_signal = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9");
  error_signal.Reshape({3, 3, 1});

  TensorType gt_mask({3, 3, 1});

  TensorType gt_then = TensorType::FromString("1, 0, 3, 0, 0, 0, 0, 8, 9");
  gt_then.Reshape({3, 3, 1});

  TensorType gt_else = TensorType::FromString("0, 2, 0, 4, 5, 6, 7, 0, 0");
  gt_else.Reshape({3, 3, 1});

  fetch::ml::ops::Switch<TensorType> op;

  std::vector<TypeParam> prediction = op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // test correct values
  ASSERT_TRUE(prediction.at(0).AllClose(gt_mask, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction.at(1).AllClose(gt_then, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction.at(2).AllClose(gt_else, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SwitchTest, back_test_broadcast_mask)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType mask = TensorType::FromString("1, 1, 0");
  mask.Reshape({1, 3, 1});

  TensorType target_input = TensorType::FromString("3, 6, 2, 1, 3, -2, 2, 1, -9");
  target_input.Reshape({3, 3, 1});

  TensorType mask_value({3, 3, 1});
  mask_value.Fill(static_cast<DataType>(-100));

  TensorType error_signal = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9");
  error_signal.Reshape({3, 3, 1});

  TensorType gt_mask({1, 3, 1});

  TensorType gt_then = TensorType::FromString("1, 2, 3, 4, 5, 6, 0, 0, 0");
  gt_then.Reshape({3, 3, 1});

  TensorType gt_else = TensorType::FromString("0, 0, 0, 0, 0, 0, 7, 8, 9");
  gt_else.Reshape({3, 3, 1});

  fetch::ml::ops::Switch<TensorType> op;

  std::vector<TypeParam> prediction = op.Backward(
      {std::make_shared<const TensorType>(mask), std::make_shared<const TensorType>(target_input),
       std::make_shared<const TensorType>(mask_value)},
      error_signal);

  // test correct values
  ASSERT_TRUE(prediction.at(0).AllClose(gt_mask, DataType{0}, DataType{0}));
  ASSERT_TRUE(prediction.at(1).AllClose(gt_then, DataType{0}, DataType{0}));
  ASSERT_TRUE(prediction.at(2).AllClose(gt_else, DataType{0}, DataType{0}));
}

}  // namespace
