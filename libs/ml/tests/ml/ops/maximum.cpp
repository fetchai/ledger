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

#include "ml/ops/maximum.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class MaximumTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(MaximumTest, MyTypes);

TYPED_TEST(MaximumTest, forward_test)
{
  using ArrayType = TypeParam;

  ArrayType data_1 = ArrayType::FromString(R"(
  	 1, -2, 3,-4, 5,-6, 7,-8;
     1,  2, 3, 4, 5, 6, 7, 8)");

  ArrayType data_2 = ArrayType::FromString(R"(
  	 8, -7, 6,-5, 4,-3, 2,-1;
    -8,  7,-6, 5,-4, 3,-2, 1)");

  ArrayType gt = ArrayType::FromString(R"(
  	 8,	-2, 6, -4, 5, -3, 7, -1;
     1,  7, 3,  5, 5,  6, 7,  8)");

  fetch::ml::ops::Maximum<ArrayType> op;

  TypeParam prediction(op.ComputeOutputShape({data_1, data_2}));
  op.Forward({data_1, data_2}, prediction);

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(MaximumTest, backward_test)
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
    0, -1, 0, -2, 3, 0, 4, 0;
    5, 0, 6, 0, 7, -7, 8, -8)");

  ArrayType gt_2 = ArrayType::FromString(R"(
    1, 0, 2, 0, 0, -3, 0, -4;
    0, -5, 0, -6, 0, 0, 0, 0)");

  ArrayType error = ArrayType::FromString(R"(
  	1, -1, 2, -2, 3, -3, 4, -4;
    5, -5, 6, -6, 7, -7, 8, -8)");

  fetch::ml::ops::Maximum<ArrayType> op;
  std::vector<ArrayType>             prediction = op.Backward({data_1, data_2}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, DataType(1e-5), DataType(1e-5)));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, DataType(1e-5), DataType(1e-5)));
}
