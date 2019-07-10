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

#include "ml/ops/sqrt.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cmath>
#include <cstdint>
#include <vector>

template <typename T>
class SqrtFloatTest : public ::testing::Test
{
};

template <typename T>
class SqrtFixedTest : public ::testing::Test
{
};

template <typename T>
class SqrtBothTest : public ::testing::Test
{
};

using FloatingPointTypes =
    ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>>;

using FixedPointTypes = ::testing::Types<fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                         fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

using BothTypes = ::testing::Types<fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                   fetch::math::Tensor<fetch::fixed_point::fp64_t>,
                                   fetch::math::Tensor<float>, fetch::math::Tensor<double>>;

TYPED_TEST_CASE(SqrtFloatTest, FloatingPointTypes);
TYPED_TEST_CASE(SqrtFixedTest, FixedPointTypes);
TYPED_TEST_CASE(SqrtBothTest, BothTypes);

TYPED_TEST(SqrtBothTest, forward_all_positive_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename ArrayType::Type;

  ArrayType data = ArrayType::FromString("0, 1, 2, 4, 10, 100");
  ArrayType gt   = ArrayType::FromString("0, 1, 1.41421356, 2, 3.1622776, 10");

  fetch::ml::ops::Sqrt<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SqrtBothTest, backward_all_positive_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename ArrayType::Type;

  ArrayType data  = ArrayType::FromString("1,   2,         4,   10,       100");
  ArrayType error = ArrayType::FromString("1,   1,         1,    2,         0");
  // 0.5 / sqrt(data) * error = gt
  ArrayType gt = ArrayType::FromString("0.5, 0.3535533, 0.25, 0.3162277, 0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<ArrayType> prediction = op.Backward({data}, error);

  ASSERT_TRUE(prediction.at(0).AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                        fetch::math::function_tolerance<DataType>()));
}

// TODO(1195): fixed point and floating point tests should be unified.
TYPED_TEST(SqrtFloatTest, forward_all_negative_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString("-1, -2, -4, -10, -100");

  fetch::ml::ops::Sqrt<TypeParam> op;

  TypeParam pred(op.ComputeOutputShape({data}));
  op.Forward({data}, pred);

  // gives NaN because sqrt of a negative number is undefined
  for (auto p_it : pred)
  {
    EXPECT_TRUE(std::isnan(p_it));
  }
}

TYPED_TEST(SqrtFixedTest, forward_all_negative_test)
{
  using ArrayType = TypeParam;

  ArrayType data = ArrayType::FromString("-1, -2, -4, -10, -100");

  fetch::ml::ops::Sqrt<TypeParam> op;

  TypeParam pred(op.ComputeOutputShape({data}));
  op.Forward({data}, pred);

  // gives NaN because sqrt of a negative number is undefined
  for (auto p_it : pred)
  {
    EXPECT_TRUE(ArrayType::Type::IsNaN(p_it));
  }
}

TYPED_TEST(SqrtFloatTest, backward_all_negative_test)
{
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString("-1, -2, -4, -10, -100");
  ArrayType error = ArrayType::FromString("1,   1,  1,   2,    0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<ArrayType> pred = op.Backward({data}, error);
  // gives NaN because sqrt of a negative number is undefined
  for (auto p_it : pred.at(0))
  {
    EXPECT_TRUE(std::isnan(p_it));
  }
}

TYPED_TEST(SqrtFixedTest, backward_all_negative_test)
{
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString("-1, -2, -4, -10, -100");
  ArrayType error = ArrayType::FromString("1,   1,  1,   2,    0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<ArrayType> pred = op.Backward({data}, error);
  // gives NaN because sqrt of a negative number is undefined
  for (auto p_it : pred.at(0))
  {
    EXPECT_TRUE(ArrayType::Type::IsNaN(p_it));
  }
}

TYPED_TEST(SqrtFloatTest, backward_zero_test)
{
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString("0,  0,    0,    0,       -0");
  ArrayType error = ArrayType::FromString("1,100,   -1,    2,        1");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<ArrayType> pred = op.Backward({data}, error);
  // gives NaN because of division by zero
  for (auto p_it : pred.at(0))
  {
    EXPECT_TRUE(std::isinf(p_it));
  }
}

TYPED_TEST(SqrtFixedTest, backward_zero_test)
{
  using ArrayType = TypeParam;

  ArrayType data  = ArrayType::FromString("0,  0,    0,    0,        0");
  ArrayType error = ArrayType::FromString("1,  1,    1,    2,        0");

  fetch::ml::ops::Sqrt<TypeParam> op;

  std::vector<ArrayType> pred = op.Backward({data}, error);
  // gives NaN because of division by zero
  for (auto p_it : pred.at(0))
  {
    EXPECT_TRUE(ArrayType::Type::IsNaN(p_it));
  }
}
