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
#include <numeric>

#include "core/fixed_point/fixed_point.hpp"
#include "core/random/lcg.hpp"
#include "math/base_types.hpp"                    // wraps NumericMax
#include "math/fundamental_operators.hpp"
#include "math/tensor.hpp"

template <typename T>
class FundamentalOperatorsUIntTypeTest : public ::testing::Test
{
};

template <typename T>
class FundamentalOperatorsIntTypeTest : public ::testing::Test
{
};

template <typename T>
class FundamentalOperatorsNonIntTypeTest : public ::testing::Test
{
};

static fetch::random::LinearCongruentialGenerator gen;

using MyUnsignIntegralTypes = ::testing::Types<std::uint32_t, std::uint64_t>;
TYPED_TEST_CASE(FundamentalOperatorsUIntTypeTest, MyUnsignIntegralTypes);

using MySignedIntegralTypes = ::testing::Types<std::int32_t, std::int64_t>;
TYPED_TEST_CASE(FundamentalOperatorsIntTypeTest, MySignedIntegralTypes);

using MyNonIntegralTypes = ::testing::Types<double, fetch::fixed_point::FixedPoint<32, 32>>;
TYPED_TEST_CASE(FundamentalOperatorsNonIntTypeTest, MyNonIntegralTypes);

///// Addition tests
// UInt
TYPED_TEST(FundamentalOperatorsUIntTypeTest, AdditionUIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Add(a, b, ret);
  EXPECT_EQ(ret, 9);
  EXPECT_EQ(fetch::math::Add(a, b), 9);


  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Add(a, b, ret);
  EXPECT_EQ(ret, 444);
  EXPECT_EQ(fetch::math::Add(a, b), 444);

  for (std::size_t i{0}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100 - i);
    fetch::math::Add(a, b, ret);
    EXPECT_EQ(ret, 100);
    EXPECT_EQ(fetch::math::Add(a, b), 100);
  }

  // test a few big random numbers
  for (std::size_t i{0}; i < 100; ++i)
  {
    // values range from 0 - std::numeric_limits<Type>::max()
    a = TypeParam(gen.AsDouble() * max_val);
    b = TypeParam(gen.AsDouble() * max_val);

    fetch::math::Add(a, b, ret);
    EXPECT_EQ(ret, a + b);
    EXPECT_EQ(fetch::math::Add(a, b), a + b);
  }
}

// Int
TYPED_TEST(FundamentalOperatorsIntTypeTest, AdditionIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Add(a, b, ret);
  EXPECT_EQ(ret, 9);
  EXPECT_EQ(fetch::math::Add(a, b), 9);


  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Add(a, b, ret);
  EXPECT_EQ(ret, 444);
  EXPECT_EQ(fetch::math::Add(a, b), 444);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam{i};
    b = TypeParam{100 - i};
    fetch::math::Add(a, b, ret);
    EXPECT_EQ(ret, 100);
    EXPECT_EQ(fetch::math::Add(a, b), 100);
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));
    b = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));

    fetch::math::Add(a, b, ret);
    EXPECT_EQ(ret, a + b);
    EXPECT_EQ(fetch::math::Add(a, b), a + b);
  }
}

// float/double/FixedPoint
TYPED_TEST(FundamentalOperatorsNonIntTypeTest, AdditionNonIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam two = TypeParam(2.0);
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam(2.156);
  b = TypeParam(7.421);
  fetch::math::Add(a, b, ret);
  EXPECT_NEAR(double(ret), 9.577, 1e-9);
  EXPECT_NEAR(double(fetch::math::Add(a, b)), 9.577, 1e-9);

  a = TypeParam(123.456);
  b = TypeParam(321.123);
  fetch::math::Add(a, b, ret);
  EXPECT_NEAR(double(ret), 444.579, 1e-9);
  EXPECT_NEAR(double(fetch::math::Add(a, b)), 444.579, 1e-9);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100 - i);

    fetch::math::Add(a, b, ret);
    EXPECT_EQ(ret, TypeParam(100.0));
    EXPECT_EQ(fetch::math::Add(a, b), TypeParam(100.0));
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);
    b = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);

    fetch::math::Add(a, b, ret);
    EXPECT_EQ(ret, a + b);
    EXPECT_EQ(fetch::math::Add(a, b), a + b);
  }
}


/// SUBTRACTION TESTS
TYPED_TEST(FundamentalOperatorsUIntTypeTest, SubtractionUIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Subtract(b, a, ret);
  EXPECT_EQ(ret, 5);
  EXPECT_EQ(fetch::math::Subtract(b, a), 5);
  fetch::math::Subtract(a, b, ret);
  EXPECT_EQ(ret, max_val - 4);
  EXPECT_EQ(fetch::math::Subtract(a, b), max_val - 4);


  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Subtract(b, a, ret);
  EXPECT_EQ(ret, 198);
  EXPECT_EQ(fetch::math::Subtract(b, a), 198);

  for (std::size_t i{0}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100);
    fetch::math::Subtract(b, a, ret);
    EXPECT_EQ(ret, 100 - i);
    EXPECT_EQ(fetch::math::Subtract(b, a), 100 - i);
  }

  // test a few big random numbers
  for (std::size_t i{0}; i < 100; ++i)
  {
    // values range from 0 - std::numeric_limits<Type>::max()
    a = TypeParam(gen.AsDouble() * max_val);
    b = TypeParam(gen.AsDouble() * max_val);

    fetch::math::Subtract(a, b, ret);
    EXPECT_EQ(ret, a - b);
    EXPECT_EQ(fetch::math::Subtract(a, b), a - b);
  }
}

TYPED_TEST(FundamentalOperatorsIntTypeTest, SubtractionIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Subtract(a, b, ret);
  EXPECT_EQ(ret, -5);
  EXPECT_EQ(fetch::math::Subtract(a, b), -5);


  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Subtract(a, b, ret);
  EXPECT_EQ(ret, -198);
  EXPECT_EQ(fetch::math::Subtract(a, b), -198);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam{i};
    b = TypeParam{100 + i};
    fetch::math::Subtract(b, a, ret);
    EXPECT_EQ(ret, 100);
    EXPECT_EQ(fetch::math::Subtract(b, a), 100);
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));
    b = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));

    fetch::math::Subtract(a, b, ret);
    EXPECT_EQ(ret, a - b);
    EXPECT_EQ(fetch::math::Subtract(a, b), a - b);
  }
}

TYPED_TEST(FundamentalOperatorsNonIntTypeTest, SubtractionNonIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam two = TypeParam(2.0);
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam(2.156);
  b = TypeParam(7.421);
  fetch::math::Subtract(a, b, ret);
  EXPECT_NEAR(double(ret), -5.265, 1e-9);
  EXPECT_NEAR(double(fetch::math::Subtract(a, b)), -5.265, 1e-9);

  a = TypeParam(123.456);
  b = TypeParam(321.123);
  fetch::math::Subtract(a, b, ret);
  EXPECT_NEAR(double(ret), -197.667, 1e-9);
  EXPECT_NEAR(double(fetch::math::Subtract(a, b)), -197.667, 1e-9);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100 + i);

    fetch::math::Subtract(b, a, ret);
    EXPECT_EQ(ret, TypeParam(100.0));
    EXPECT_EQ(fetch::math::Subtract(b, a), TypeParam(100.0));
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);
    b = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);

    fetch::math::Subtract(a, b, ret);
    EXPECT_EQ(ret, a - b);
    EXPECT_EQ(fetch::math::Subtract(a, b), a - b);
  }
}


/// Multiplication tests
TYPED_TEST(FundamentalOperatorsUIntTypeTest, MultiplicationUIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Multiply(b, a, ret);
  EXPECT_EQ(ret, 14);
  EXPECT_EQ(fetch::math::Multiply(b, a), 14);

  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Multiply(b, a, ret);
  EXPECT_EQ(ret, 39483);
  EXPECT_EQ(fetch::math::Multiply(b, a), 39483);

  for (std::size_t i{0}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100);
    fetch::math::Multiply(b, a, ret);
    EXPECT_EQ(ret, 100 * i);
    EXPECT_EQ(fetch::math::Multiply(b, a), 100 * i);
  }

  // test a few big random numbers
  for (std::size_t i{0}; i < 100; ++i)
  {
    // values range from 0 - std::numeric_limits<Type>::max()
    a = TypeParam(gen.AsDouble() * max_val);
    b = TypeParam(gen.AsDouble() * max_val);

    fetch::math::Multiply(a, b, ret);
    EXPECT_EQ(ret, a * b);
    EXPECT_EQ(fetch::math::Multiply(a, b), a * b);
  }
}


TYPED_TEST(FundamentalOperatorsIntTypeTest, MultiplicationIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Multiply(a, b, ret);
  EXPECT_EQ(ret, 14);
  EXPECT_EQ(fetch::math::Multiply(a, b), 14);


  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Multiply(a, b, ret);
  EXPECT_EQ(ret, 39483);
  EXPECT_EQ(fetch::math::Multiply(a, b), 39483);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam{i};
    b = TypeParam{100};
    fetch::math::Multiply(b, a, ret);
    EXPECT_EQ(ret, 100 * i);
    EXPECT_EQ(fetch::math::Multiply(b, a), 100 * i);
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));
    b = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));

    fetch::math::Multiply(a, b, ret);
    EXPECT_EQ(ret, a * b);
    EXPECT_EQ(fetch::math::Multiply(a, b), a * b);
  }
}


TYPED_TEST(FundamentalOperatorsNonIntTypeTest, MultiplicationNonIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam two = TypeParam(2.0);
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam(2.156);
  b = TypeParam(-7.421);
  fetch::math::Multiply(a, b, ret);
  EXPECT_NEAR(double(ret), -15.999676, 1e-7);
  EXPECT_NEAR(double(fetch::math::Multiply(a, b)), -15.999676, 1e-7);

  a = TypeParam(-123.456);
  b = TypeParam(-321.123);
  fetch::math::Multiply(a, b, ret);
  EXPECT_NEAR(double(ret), 39644.561088, 1e-7);
  EXPECT_NEAR(double(fetch::math::Multiply(a, b)), 39644.561088, 1e-7);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100);

    fetch::math::Multiply(b, a, ret);
    EXPECT_EQ(ret, TypeParam(100.0 * i));
    EXPECT_EQ(fetch::math::Multiply(b, a), TypeParam(100.0 * i));
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);
    b = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);

    fetch::math::Multiply(a, b, ret);
    EXPECT_EQ(ret, a * b);
    EXPECT_EQ(fetch::math::Multiply(a, b), a * b);
  }
}


// DIVISION TESTS
TYPED_TEST(FundamentalOperatorsUIntTypeTest, DivisionUIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Divide(a, b, ret);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(fetch::math::Divide(a, b), 0);
  fetch::math::Divide(b, a, ret);
  EXPECT_EQ(ret, 3);
  EXPECT_EQ(fetch::math::Divide(b, a), 3);

  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Divide(a, b, ret);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(fetch::math::Divide(a, b), 0);
  fetch::math::Divide(b, a, ret);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(fetch::math::Divide(b, a), 2);

  for (std::size_t i{0}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100);
    fetch::math::Divide(a, b, ret);
    EXPECT_EQ(ret, i / 100);
    EXPECT_EQ(fetch::math::Divide(a, b), i / 100);
  }

  // test a few big random numbers
  for (std::size_t i{0}; i < 100; ++i)
  {
    // values range from 0 - std::numeric_limits<Type>::max()
    a = TypeParam(gen.AsDouble() * max_val);
    b = TypeParam(gen.AsDouble() * max_val);

    fetch::math::Divide(a, b, ret);
    EXPECT_EQ(ret, a / b);
    EXPECT_EQ(fetch::math::Divide(a, b), a / b);
  }
}


TYPED_TEST(FundamentalOperatorsIntTypeTest, DivisionIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();


  // test a few small fixed values
  a = TypeParam{2};
  b = TypeParam{7};
  fetch::math::Divide(a, b, ret);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(fetch::math::Divide(a, b), 0);
  fetch::math::Divide(b, a, ret);
  EXPECT_EQ(ret, 3);
  EXPECT_EQ(fetch::math::Divide(b, a), 3);

  a = TypeParam{123};
  b = TypeParam{321};
  fetch::math::Divide(a, b, ret);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(fetch::math::Divide(a, b), 0);
  fetch::math::Divide(b, a, ret);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(fetch::math::Divide(b, a), 2);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam{i};
    b = TypeParam{100};
    fetch::math::Divide(a, b, ret);
    EXPECT_EQ(ret, i / 100);
    EXPECT_EQ(fetch::math::Divide(a, b), i / 100);
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));
    b = TypeParam((gen.AsDouble() * max_val) - (max_val / 2));

    fetch::math::Divide(a, b, ret);
    EXPECT_EQ(ret, a / b);
    EXPECT_EQ(fetch::math::Divide(a, b), a / b);
  }
}


TYPED_TEST(FundamentalOperatorsNonIntTypeTest, DivisionNonIntTest)
{
  TypeParam a;
  TypeParam b;
  TypeParam ret;
  TypeParam two = TypeParam(2.0);
  TypeParam max_val = fetch::math::NumericMax<TypeParam>();

  // test a few small fixed values
  a = TypeParam(2.156);
  b = TypeParam(-7.421);
  fetch::math::Divide(a, b, ret);
  EXPECT_NEAR(double(ret), -0.29052688, 1e-7);
  EXPECT_NEAR(double(fetch::math::Divide(a, b)), -0.29052688, 1e-7);

  a = TypeParam(-123.456);
  b = TypeParam(-321.123);
  fetch::math::Divide(a, b, ret);
  EXPECT_NEAR(double(ret), 0.3844508, 1e-7);
  EXPECT_NEAR(double(fetch::math::Divide(a, b)), 0.3844508, 1e-7);

  for (int i{-100}; i < 100; ++i)
  {
    a = TypeParam(i);
    b = TypeParam(100);

    fetch::math::Divide(a, b, ret);
    EXPECT_EQ(ret, TypeParam(i) / TypeParam(100));
    EXPECT_EQ(fetch::math::Divide(a, b), TypeParam(i) / TypeParam(100));
  }

  // test a few big numbers
  for (int i{0}; i < 100; ++i)
  {
    // values range from - half max to max
    a = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);
    b = (TypeParam(gen.AsDouble()) * max_val) - (max_val / two);

    fetch::math::Divide(a, b, ret);
    EXPECT_EQ(ret, a / b);
    EXPECT_EQ(fetch::math::Divide(a, b), a / b);
  }
}

