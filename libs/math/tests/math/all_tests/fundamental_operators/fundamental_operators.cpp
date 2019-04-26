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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "core/fixed_point/fixed_point.hpp"
#include "core/random/lcg.hpp"
#include "math/fundamental_operators.hpp"
#include "math/tensor.hpp"

template <typename T>
class FundamentalOperatorsDataTypeTest : public ::testing::Test
{
};
template <typename T>
class FundamentalOperatorsArrayTypeTest : public ::testing::Test
{
};
static fetch::random::LinearCongruentialGenerator gen;

using MyDataTypes = ::testing::Types<int32_t, int64_t, std::uint32_t, std::uint64_t, float, double,
                                     fetch::fixed_point::FixedPoint<16, 16>,
                                     fetch::fixed_point::FixedPoint<32, 32>>;
TYPED_TEST_CASE(FundamentalOperatorsDataTypeTest, MyDataTypes);

using MyArrayTypes =
    ::testing::Types<fetch::math::Tensor<int32_t>, fetch::math::Tensor<int64_t>,
                     fetch::math::Tensor<std::uint32_t>, fetch::math::Tensor<std::uint64_t>,
                     fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                     fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                     fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(FundamentalOperatorsArrayTypeTest, MyArrayTypes);

TYPED_TEST(FundamentalOperatorsDataTypeTest, AdditionTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;

  for (std::size_t i{0}; i < 1000; ++i)
  {
    auto tmp_val_1 = gen.AsDouble();
    auto tmp_val_2 = gen.AsDouble();

    a = TypeParam(tmp_val_1);
    b = TypeParam(tmp_val_2);
    fetch::math::Add(a, b, ret);
    EXPECT_EQ(ret, a + b);

    EXPECT_EQ(fetch::math::Add(a, b), a + b);
  }
}

TYPED_TEST(FundamentalOperatorsArrayTypeTest, AdditionTest)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  TypeParam a{100};
  TypeParam b{100};
  TypeParam ret{100};
  TypeParam gt{100};

  for (SizeType i{0}; i < 1000; ++i)
  {
    for (SizeType j{0}; j < 100; ++j)
    {
      auto tmp_val_1 = gen.AsDouble();
      auto tmp_val_2 = gen.AsDouble();

      a.At(j)  = DataType(tmp_val_1);
      b.At(j)  = DataType(tmp_val_2);
      gt.At(j) = DataType(tmp_val_1) + DataType(tmp_val_2);
    }
    fetch::math::Add(a, b, ret);
    EXPECT_TRUE(ret.AllClose(gt));
    EXPECT_TRUE(gt.AllClose(fetch::math::Add(a, b)));
  }
}

TYPED_TEST(FundamentalOperatorsDataTypeTest, SubtractionTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;

  for (std::size_t i{0}; i < 1000; ++i)
  {
    auto tmp_val_1 = gen.AsDouble();
    auto tmp_val_2 = gen.AsDouble();

    a = TypeParam(tmp_val_1);
    b = TypeParam(tmp_val_2);
    fetch::math::Subtract(a, b, ret);
    EXPECT_EQ(ret, a - b);

    EXPECT_EQ(fetch::math::Subtract(a, b), a - b);
  }
}

TYPED_TEST(FundamentalOperatorsArrayTypeTest, SubtractionTest)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  TypeParam a{100};
  TypeParam b{100};
  TypeParam ret{100};
  TypeParam gt{100};

  for (SizeType i{0}; i < 1000; ++i)
  {
    for (SizeType j{0}; j < 100; ++j)
    {
      auto tmp_val_1 = gen.AsDouble();
      auto tmp_val_2 = gen.AsDouble();

      a.At(j)  = DataType(tmp_val_1);
      b.At(j)  = DataType(tmp_val_2);
      gt.At(j) = DataType(tmp_val_1) - DataType(tmp_val_2);
    }
    fetch::math::Subtract(a, b, ret);
    EXPECT_TRUE(ret.AllClose(gt));
    EXPECT_TRUE(gt.AllClose(fetch::math::Subtract(a, b)));
  }
}

TYPED_TEST(FundamentalOperatorsDataTypeTest, MultiplicationTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;

  for (std::size_t i{0}; i < 1000; ++i)
  {
    auto tmp_val_1 = gen.AsDouble();
    auto tmp_val_2 = gen.AsDouble();

    a = TypeParam(tmp_val_1);
    b = TypeParam(tmp_val_2);
    fetch::math::Multiply(a, b, ret);
    EXPECT_EQ(ret, a * b);

    EXPECT_EQ(fetch::math::Multiply(a, b), a * b);
  }
}

TYPED_TEST(FundamentalOperatorsArrayTypeTest, MultiplicationTest)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  TypeParam a{100};
  TypeParam b{100};
  TypeParam ret{100};
  TypeParam gt{100};

  for (SizeType i{0}; i < 1000; ++i)
  {
    for (SizeType j{0}; j < 100; ++j)
    {
      auto tmp_val_1 = gen.AsDouble();
      auto tmp_val_2 = gen.AsDouble();

      a.At(j)  = DataType(tmp_val_1);
      b.At(j)  = DataType(tmp_val_2);
      gt.At(j) = DataType(tmp_val_1) * DataType(tmp_val_2);
    }
    fetch::math::Multiply(a, b, ret);
    EXPECT_TRUE(ret.AllClose(gt));
    EXPECT_TRUE(gt.AllClose(fetch::math::Multiply(a, b)));
  }
}

TYPED_TEST(FundamentalOperatorsDataTypeTest, DivisionTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;

  for (std::size_t i{0}; i < 1000; ++i)
  {
    auto tmp_val_1 = gen.AsDouble();
    auto tmp_val_2 = gen.AsDouble();

    a = TypeParam(tmp_val_1);
    b = TypeParam(tmp_val_2) + TypeParam(1);  // we just do this to avoid dividing by zero
    fetch::math::Divide(a, b, ret);
    EXPECT_EQ(ret, a / b);

    EXPECT_EQ(fetch::math::Divide(a, b), a / b);
  }
}

TYPED_TEST(FundamentalOperatorsArrayTypeTest, DivisionTest)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  TypeParam a{100};
  TypeParam b{100};
  TypeParam ret{100};
  TypeParam gt{100};

  for (SizeType i{0}; i < 1000; ++i)
  {
    for (SizeType j{0}; j < 100; ++j)
    {
      auto tmp_val_1 = gen.AsDouble();
      auto tmp_val_2 = gen.AsDouble();

      a.At(j)  = DataType(tmp_val_1);
      b.At(j)  = DataType(tmp_val_2) + DataType(1);
      gt.At(j) = DataType(tmp_val_1) / (DataType(tmp_val_2) + DataType(1));
    }
    fetch::math::Divide(a, b, ret);
    EXPECT_TRUE(ret.AllClose(gt));
    EXPECT_TRUE(gt.AllClose(fetch::math::Divide(a, b)));
  }
}
