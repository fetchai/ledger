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

#include "ml/ops/abs.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class AbsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(AbsTest, MyTypes);

TYPED_TEST(AbsTest, forward_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString(R"(
  	 1, -2, 3,-4, 5,-6, 7,-8;
     1,  2, 3, 4, 5, 6, 7, 8)");

  ArrayType gt = ArrayType::FromString(R"(
    1, 2, 3, 4, 5, 6, 7, 8;
    1, 2, 3, 4, 5, 6, 7, 8)");

  fetch::ml::ops::Abs<ArrayType> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(AbsTest, backward_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString(R"(
  	 1, -2, 3,-4, 5,-6, 7,-8;
     1,  2, 3, 4, 5, 6, 7, 8)");

  ArrayType gt = ArrayType::FromString(R"(
  	1, 1, 2, 2, 3, 3, 4, 4;
    5, -5, 6, -6, 7, -7, 8, -8)");

  ArrayType error = ArrayType::FromString(R"(
  	1, -1, 2, -2, 3, -3, 4, -4;
    5, -5, 6, -6, 7, -7, 8, -8)");

  fetch::ml::ops::Abs<ArrayType> op;
  std::vector<ArrayType>         prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, DataType(1e-5), DataType(1e-5)));
}
