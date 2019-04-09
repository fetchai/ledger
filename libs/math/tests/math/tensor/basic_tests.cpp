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

#include <iomanip>
#include <iostream>
#include "math/tensor.hpp"
#include <gtest/gtest.h>
#include "core/fixed_point/fixed_point.hpp"
#include "meta/type_traits.hpp"

using namespace fetch::math;

/// template for producing a random array of FixedPoints
template <std::size_t I, std::size_t F>
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

/// template for producing a random array of integer types
template <typename T>
fetch::meta::IfIsInteger<T, Tensor<T, fetch::memory::SharedArray<T>>> RandomArray(
    std::size_t n, T adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<T, fetch::memory::SharedArray<T>>  a1(n);

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

/// template for producing a random array of float types
template <typename T>
fetch::meta::IfIsFloat<T, Tensor<T, fetch::memory::SharedArray<T>>> RandomArray(
    std::size_t n, T adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<T, fetch::memory::SharedArray<T>>  a1(n);

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

template <typename T>
void equal_test()
{
  std::size_t       n            = 10000;
  Tensor<T> test_array   = RandomArray(n, T(0));
  Tensor<T> result_array = test_array;

  ASSERT_TRUE(result_array.AllClose(test_array));
}

TEST(shapeless_array_tests, int_equals)
{
  equal_test<int>();
}
TEST(shapeless_array_tests, uint32_t_equals)
{
  equal_test<uint32_t>();
}
TEST(shapeless_array_tests, float_equals)
{
  equal_test<float>();
}
TEST(shapeless_array_tests, double_equals)
{
  equal_test<double>();
}
TEST(shapeless_array_tests, fp32_equals)
{
  equal_test<fetch::fixed_point::FixedPoint<32, 32>>();
}

////////////
/// copy ///
////////////

template <typename T>
void copy_test()
{
  std::size_t       n          = 10000;
  Tensor<T> test_array = RandomArray(n, T(0));
  Tensor<T> result_array(n);
  result_array.Copy(test_array);

  ASSERT_TRUE(result_array.AllClose(test_array));
}

TEST(shapeless_array_tests, int_copy)
{
  copy_test<int>();
}
TEST(shapeless_array_tests, size_t_copy)
{
  copy_test<uint32_t>();
}
TEST(shapeless_array_tests, float_copy)
{
  copy_test<float>();
}
TEST(shapeless_array_tests, double_copy)
{
  copy_test<double>();
}
TEST(shapeless_array_tests, fp32_copy)
{
  copy_test<fetch::fixed_point::FixedPoint<32, 32>>();
}

//////////////////
/// + operator ///
//////////////////

template <typename T>
void plus_test()
{
  std::size_t       n            = 10;
  Tensor<T> test_array   = RandomArray(n, T(0));
  Tensor<T> test_array_2 = RandomArray(n, T(0));
  Tensor<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] + test_array_2[j];
  }

  ASSERT_TRUE(result_array.AllClose(test_array + test_array_2));
}

TEST(shapeless_array_tests, integer_plus)
{
  plus_test<int>();
}
TEST(shapeless_array_tests, uint32_t_plus)
{
  plus_test<uint32_t>();
}
TEST(shapeless_array_tests, float_plus)
{
  plus_test<float>();
}
TEST(shapeless_array_tests, double_plus)
{
  plus_test<double>();
}
TEST(shapeless_array_tests, fp32_plus)
{
  plus_test<fetch::fixed_point::FixedPoint<32, 32>>();
}

//////////////////
/// - operator ///
//////////////////

template <typename T>
void sub_test()
{
  std::size_t       n            = 10000;
  Tensor<T> test_array   = RandomArray(n, T(0));
  Tensor<T> test_array_2 = RandomArray(n, T(0));
  Tensor<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] - test_array_2[j];
  }

  ASSERT_TRUE(result_array.AllClose(test_array - test_array_2));
}

TEST(shapeless_array_tests, integer_sub)
{
  sub_test<int>();
}
TEST(shapeless_array_tests, size_t_sub)
{
  sub_test<uint32_t>();
}
TEST(shapeless_array_tests, float_sub)
{
  sub_test<float>();
}
TEST(shapeless_array_tests, double_sub)
{
  sub_test<double>();
}
TEST(shapeless_array_tests, fp32_sub)
{
  sub_test<fetch::fixed_point::FixedPoint<32, 32>>();
}

//////////////////
/// * operator ///
//////////////////

template <typename T>
void mult_test()
{
  std::size_t       n            = 10000;
  Tensor<T> test_array   = RandomArray(n, T(0));
  Tensor<T> test_array_2 = RandomArray(n, T(0));
  Tensor<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array.At(j) * test_array_2.At(j);
  }

  ASSERT_TRUE(result_array.AllClose(test_array * test_array_2));
}

TEST(shapeless_array_tests, integer_mult_test)
{
  mult_test<int>();
}
TEST(shapeless_array_tests, size_t_mult_test)
{
  mult_test<uint32_t>();
}
TEST(shapeless_array_tests, float_mult_test)
{
  mult_test<float>();
}
TEST(shapeless_array_tests, double_mult_test)
{
  mult_test<double>();
}
TEST(shapeless_array_tests, fixed_mult_test_32)
{
  mult_test<fetch::fixed_point::FixedPoint<32, 32>>();
}

//////////////////
/// / operator ///
//////////////////

template <typename T>
void div_test()
{
  std::size_t       n            = 12;
  Tensor<T> test_array   = RandomArray(n, T(1));
  Tensor<T> test_array_2 = RandomArray(n, T(1));
  Tensor<T> result_array = RandomArray(n, T(1));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] / test_array_2[j];
  }

  Tensor<T> tmp(n);
  tmp = test_array / test_array_2;

  ASSERT_TRUE(result_array.AllClose(tmp));
}

TEST(shapeless_array_tests, integer_div_test)
{
  div_test<int>();
}
TEST(shapeless_array_tests, size_t_div_test)
{
  div_test<uint32_t>();
}
TEST(shapeless_array_tests, float_div_test)
{
  div_test<float>();
}
TEST(shapeless_array_tests, double_div_test)
{
  div_test<double>();
}
TEST(shapeless_array_tests, fixed_div_test_32)
{
  div_test<fetch::fixed_point::FixedPoint<32, 32>>();
}

///////////////////////////
///// equality operator ///
///////////////////////////

template <typename T>
void is_equal_test()
{
  std::size_t       n          = 10000;
  Tensor<T> test_array = RandomArray(n, T(0));
  Tensor<T> test_array_2(n);
  test_array_2 = test_array.Copy();

  ASSERT_TRUE(test_array == test_array_2);
}

TEST(shapeless_array_tests, integer_is_equal_test)
{
  is_equal_test<int>();
}
TEST(shapeless_array_tests, size_t_is_equal_test)
{
  is_equal_test<uint32_t>();
}
TEST(shapeless_array_tests, float_is_equal_test)
{
  is_equal_test<float>();
}
TEST(shapeless_array_tests, double_is_equal_test)
{
  is_equal_test<double>();
}

/////////////////////////////
///// not-equals operator ///
/////////////////////////////

template <typename T>
void is_not_equal_test()
{
  std::size_t       n          = 10000;
  Tensor<T> test_array = RandomArray(n, T(0));
  Tensor<T> test_array_2(n);

  for (std::size_t j = 0; j < test_array.size(); ++j)
  {
    test_array_2.Set(j, test_array.At(j) + T(1));
  }

  ASSERT_TRUE(test_array != test_array_2);
}

TEST(shapeless_array_tests, integer_is_not_equal_test)
{
  is_not_equal_test<int>();
}
TEST(shapeless_array_tests, size_t_is_not_equal_test)
{
  is_not_equal_test<uint32_t>();
}
TEST(shapeless_array_tests, float_is_not_equal_test)
{
  is_not_equal_test<float>();
}
TEST(shapeless_array_tests, double_is_not_equal_test)
{
  is_not_equal_test<double>();
}

/////////////////////
///// += operator ///
/////////////////////

template <typename T>
void plus_equals_test()
{
  std::size_t       n            = 10000;
  Tensor<T> test_array   = RandomArray(n, T(0));
  Tensor<T> test_array_2 = test_array;
  Tensor<T> result_array = test_array * T(2);

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    test_array_2[j] += test_array[j];
  }
  ASSERT_TRUE(test_array_2.AllClose(result_array));
}

TEST(shapeless_array_tests, integer_plus_equals_test)
{
  plus_equals_test<int>();
}
TEST(shapeless_array_tests, size_t_plus_equals_test)
{
  plus_equals_test<uint32_t>();
}
TEST(shapeless_array_tests, float_plus_equals_test)
{
  plus_equals_test<float>();
}
TEST(shapeless_array_tests, double_plus_equals_test)
{
  plus_equals_test<double>();
}

/////////////////////
///// -= operator ///
/////////////////////

template <typename T>
void minus_equals_test()
{
  std::size_t       n            = 10000;
  Tensor<T> test_array   = RandomArray(n, T(0));
  Tensor<T> test_array_2 = test_array * T(2);
  Tensor<T> result_array = test_array;

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    test_array_2[j] -= test_array[j];
  }
  ASSERT_TRUE(test_array_2.AllClose(result_array));
}

TEST(shapeless_array_tests, integer_minus_equals_test)
{
  minus_equals_test<int>();
}
TEST(shapeless_array_tests, size_t_minus_equals_test)
{
  minus_equals_test<uint32_t>();
}
TEST(shapeless_array_tests, float_minus_equals_test)
{
  minus_equals_test<float>();
}
TEST(shapeless_array_tests, double_minus_equals_test)
{
  minus_equals_test<double>();
}
