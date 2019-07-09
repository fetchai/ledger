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

#include "ml/ops/divide.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

namespace {
template <typename T>
class DivideTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(DivideTest, MyTypes);

TYPED_TEST(DivideTest, forward_test)
{
  using ArrayType = TypeParam;

  ArrayType data_1 = ArrayType::FromString(R"(
  	 1, -2, 3,-4, 5,-6, 7,-8;
     1,  2, 3, 4, 5, 6, 7, 8)");

  ArrayType data_2 = ArrayType::FromString(R"(
  	 8, -7, 6,-5, 4,-3, 2,-1;
    -8,  7,-6, 5,-4, 3,-2, 1)");

  ArrayType gt = ArrayType::FromString(R"(
  	0.125,	0.28571,	0.5,	0.8,	1.25,	2,	3.5,	8;
    -0.125, 0.28571,	-0.5,	0.8,	-1.25,	2,	-3.5,	8)");

  fetch::ml::ops::Divide<ArrayType> op;

  TypeParam prediction(op.ComputeOutputShape({data_1, data_2}));
  op.Forward({data_1, data_2}, prediction);

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(DivideTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data_1 = ArrayType::FromString(R"(
  	 1, -2, 3,-4, 5,-6, 7,-8;
     1,  2, 3, 4, 5, 6, 7, 8)");

  ArrayType data_2 = ArrayType::FromString(R"(
  	 8, -7, 6,-5, 4,-3, 2,-1;
    -8,  7,-6, 5,-4, 3,-2, 1)");

  ArrayType gt_1 = ArrayType::FromString(R"(
    0.125,	0.1428571,	0.33333,	0.4,	0.75,	1,	2,	4;
    -0.625,	-0.714286,	-1,	-1.2,	-1.75,	-2.33333,	-4,	-8)");

  ArrayType gt_2 = ArrayType::FromString(R"(
    0.015625,	0.04082,	0.1666667,	0.32,	0.9375,	2,	7,	32;
    0.078125,  -0.20408,	0.5,	-0.96,	2.1875,	-4.6666667,	14,	-64)");

  ArrayType error = ArrayType::FromString(R"(
  	1, -1, 2, -2, 3, -3, 4, -4;
    5, -5, 6, -6, 7, -7, 8, -8)");

  fetch::ml::ops::Divide<ArrayType> op;
  std::vector<ArrayType>            prediction = op.Backward({data_1, data_2}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, DataType(1e-5), DataType(1e-5)));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, DataType(1e-5), DataType(1e-5)));
}
}  // namespace
