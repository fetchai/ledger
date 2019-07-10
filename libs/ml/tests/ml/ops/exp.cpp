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

#include "ml/ops/exp.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class ExpTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                 fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

TYPED_TEST_CASE(ExpTest, MyTypes);

TYPED_TEST(ExpTest, forward_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType data = ArrayType::FromString(
      " 0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  ArrayType gt = ArrayType::FromString(
      "1,	0.13534,	20.08554,	0.018316,	148.41316,	0.00248,	"
      "1096.63316,	0.00034;"
      "0.36788,	7.38906,	0.049787,	54.59815,	0.0067379,	403.428793,	"
      "0.000912,	2980.95799");

  fetch::ml::ops::Exp<ArrayType> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ExpTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString(
      "0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  ArrayType gt = ArrayType::FromString(
      "1,	-0.13533,	40.17107,	-0.03663,	445.23948,	-0.0074363,	"
      "4386.5326,	-0.00134;"
      "1.8394,	-36.94528,	0.29872,	-327.58890,	0.047166,	-2824.00155,	"
      "0.007295,	-23847.663896");

  ArrayType error = ArrayType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Exp<ArrayType> op;
  std::vector<ArrayType>         prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}
