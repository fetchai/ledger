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

#include "math/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include "meta/type_traits.hpp"
#include <gtest/gtest.h>
#include <iostream>

template <typename T>
class TensorBasicTests : public ::testing::Test
{
};

using MyTypes = ::testing::Types<std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, float,
                                 double, fetch::fixed_point::FixedPoint<32, 32>>;
TYPED_TEST_CASE(TensorBasicTests, MyTypes);

using namespace fetch::math;

// template for producing a random array of FixedPoints
template <std::uint16_t I, std::uint16_t F>
Tensor<fetch::fixed_point::FixedPoint<I, F>,
       fetch::memory::SharedArray<fetch::fixed_point::FixedPoint<I, F>>>
RandomArray(std::size_t n, fetch::fixed_point::FixedPoint<I, F> adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<fetch::fixed_point::FixedPoint<I, F>,
         fetch::memory::SharedArray<fetch::fixed_point::FixedPoint<I, F>>>
      a1(n);

  fetch::fixed_point::FixedPoint<I, F> rn{0};
  for (std::size_t i = 0; i < n; ++i)
  {
    rn       = fetch::fixed_point::FixedPoint<I, F>(gen.AsDouble());
    a1.At(i) = rn + adj;
  }
  return a1;
}

// template for producing a random array of integer types
template <typename T>
fetch::meta::IfIsInteger<T, Tensor<T, fetch::memory::SharedArray<T>>> RandomArray(std::size_t n,
                                                                                  T           adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<T, fetch::memory::SharedArray<T>>          a1(n);

  // because random numbers are between 0 and 1 which doesn't work for integers
  double scale = 1000;

  T rn{0};
  for (std::size_t i = 0; i < n; ++i)
  {
    rn       = T(gen.AsDouble() * scale);
    a1.At(i) = rn + adj;
  }
  return a1;
}

// template for producing a random array of float types
template <typename T>
fetch::meta::IfIsFloat<T, Tensor<T, fetch::memory::SharedArray<T>>> RandomArray(std::size_t n,
                                                                                T           adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<T, fetch::memory::SharedArray<T>>          a1(n);

  T rn{0};
  for (std::size_t i = 0; i < n; ++i)
  {
    rn       = T(gen.AsDouble());
    a1.At(i) = rn + adj;
  }
  return a1;
}

///////////////////////////
/// assignment operator ///
///////////////////////////

TYPED_TEST(TensorBasicTests, equals)
{
  std::size_t       n            = 10000;
  Tensor<TypeParam> test_array   = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> result_array = test_array;

  ASSERT_TRUE(result_array.AllClose(test_array));
}

////////////
/// copy ///
////////////

TYPED_TEST(TensorBasicTests, copy)
{
  std::size_t       n          = 10000;
  Tensor<TypeParam> test_array = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> result_array(n);
  result_array.Copy(test_array);

  ASSERT_TRUE(result_array.AllClose(test_array));
}

//////////////////
/// + operator ///
//////////////////

TYPED_TEST(TensorBasicTests, plus)
{
  std::size_t       n            = 10;
  Tensor<TypeParam> test_array   = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> test_array_2 = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> result_array = RandomArray(n, TypeParam(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] + test_array_2[j];
  }

  ASSERT_TRUE(result_array.AllClose(test_array + test_array_2));
}

//////////////////
/// - operator ///
//////////////////

TYPED_TEST(TensorBasicTests, sub)
{
  std::size_t       n            = 10000;
  Tensor<TypeParam> test_array   = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> test_array_2 = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> result_array = RandomArray(n, TypeParam(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] - test_array_2[j];
  }

  ASSERT_TRUE(result_array.AllClose(test_array - test_array_2));
}

//////////////////
/// * operator ///
//////////////////

TYPED_TEST(TensorBasicTests, mult_test)
{
  std::size_t       n            = 10000;
  Tensor<TypeParam> test_array   = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> test_array_2 = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> result_array = RandomArray(n, TypeParam(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array.At(j) * test_array_2.At(j);
  }

  ASSERT_TRUE(result_array.AllClose(test_array * test_array_2));
}

//////////////////
/// / operator ///
//////////////////

TYPED_TEST(TensorBasicTests, div_test)
{
  std::size_t       n            = 12;
  Tensor<TypeParam> test_array   = RandomArray(n, TypeParam(1));
  Tensor<TypeParam> test_array_2 = RandomArray(n, TypeParam(1));
  Tensor<TypeParam> result_array = RandomArray(n, TypeParam(1));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] / test_array_2[j];
  }

  Tensor<TypeParam> tmp(n);
  tmp = test_array / test_array_2;

  ASSERT_TRUE(result_array.AllClose(tmp));
}

///////////////////////////
///// equality operator ///
///////////////////////////

TYPED_TEST(TensorBasicTests, is_equal_test)
{
  std::size_t       n          = 10000;
  Tensor<TypeParam> test_array = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> test_array_2(n);
  test_array_2 = test_array.Copy();

  ASSERT_TRUE(test_array == test_array_2);
}

/////////////////////////////
///// not-equals operator ///
/////////////////////////////

TYPED_TEST(TensorBasicTests, is_not_equal_test)
{
  using SizeType               = typename Tensor<TypeParam>::SizeType;
  std::size_t       n          = 10000;
  Tensor<TypeParam> test_array = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> test_array_2(n);

  for (std::size_t j = 0; j < test_array.size(); ++j)
  {
    test_array_2.Set(SizeType{j}, test_array.At(j) + TypeParam(1));
  }

  ASSERT_TRUE(test_array != test_array_2);
}

/////////////////////
///// += operator ///
/////////////////////

TYPED_TEST(TensorBasicTests, plus_equals_test)
{
  std::size_t       n            = 10000;
  Tensor<TypeParam> test_array   = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> test_array_2 = test_array;
  Tensor<TypeParam> result_array = test_array * TypeParam(2);

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    test_array_2[j] += test_array[j];
  }
  ASSERT_TRUE(test_array_2.AllClose(result_array));
}

/////////////////////
///// -= operator ///
/////////////////////

TYPED_TEST(TensorBasicTests, minus_equals_test)
{
  std::size_t       n            = 10000;
  Tensor<TypeParam> test_array   = RandomArray(n, TypeParam(0));
  Tensor<TypeParam> test_array_2 = test_array * TypeParam(2);
  Tensor<TypeParam> result_array = test_array;

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    test_array_2[j] -= test_array[j];
  }
  ASSERT_TRUE(test_array_2.AllClose(result_array));
}
