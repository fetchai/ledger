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
#include "ml/ops/exp.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

template <typename T>
class ExpTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ExpTest, fetch::math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(ExpTest, forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString(
      " 0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  TensorType gt = TensorType::FromString(
      "1, 0.135335283236613, 20.0855369231877, 0.018315638888734, 148.413159102577, "
      "0.002478752176666, 1096.63315842846, 0.000335462627903;"
      "0.367879441171442, 7.38905609893065, 0.049787068367864, 54.5981500331442, "
      "0.006737946999085, 403.428793492735, 0.000911881965555, 2980.95798704173");

  fetch::ml::ops::Exp<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ExpTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString(
      "0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  TensorType gt = TensorType::FromString(
      "1, -0.135335283236613, 40.1710738463753, -0.036631277777468, 445.23947730773, "
      "-0.007436256529999, 4386.53263371383, -0.00134185051161;"
      "1.83939720585721, -36.9452804946533, 0.298722410207184, -327.588900198865, "
      "0.047165628993598, -2824.00155444915, 0.007295055724436, -23847.6638963338");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Exp<TensorType> op;
  std::vector<TensorType>         prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

}  // namespace
