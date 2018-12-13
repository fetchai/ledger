//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "math/kernels/sign.hpp"
#include "math/shapeless_array.hpp"
#include <gtest/gtest.h>

#include "core/fixed_point/fixed_point.hpp"

using namespace fetch::math;

template <typename T>
ShapelessArray<T, fetch::memory::SharedArray<T>> RandomArray(std::size_t n, T adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ShapelessArray<T, fetch::memory::SharedArray<T>>  a1(n);

  // because random numbers are between 0 and 1 which doesn't work for integers
  double scale = 1000;

  T rn;
  for (std::size_t i = 0; i < n; ++i)
  {
    rn       = T(gen.AsDouble() * scale);
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
  ShapelessArray<T> test_array   = RandomArray<T>(n, T(0));
  ShapelessArray<T> result_array = test_array;

  ASSERT_TRUE(result_array.AllClose(test_array));
}


TEST(ndarray, int_equals_test)
{
  equal_test<int>();
}
TEST(ndarray, uint32_t_equals_test)
{
  equal_test<uint32_t>();
}
TEST(ndarray, float_equals_test)
{
  equal_test<float>();
}
TEST(ndarray, double_equals_test)
{
  equal_test<double>();
}
//TEST(ndarray, fixed_equals_test)
//{
//  equal_test<fetch::fixed_point::FixedPoint<16, 16>>();
//}


////////////
/// copy ///
////////////

template <typename T>
void copy_test()
{
  std::size_t       n          = 10000;
  ShapelessArray<T> test_array = RandomArray<T>(n, 0);
  ShapelessArray<T> result_array(n);
  result_array.Copy(test_array);

  ASSERT_TRUE(result_array.AllClose(test_array));
}

TEST(ndarray, int_copy_test)
{
  equal_test<int>();
}
TEST(ndarray, size_t_copy_test)
{
  equal_test<uint32_t>();
}
TEST(ndarray, float_copy_test)
{
  equal_test<float>();
}
TEST(ndarray, double_copy_test)
{
  equal_test<double>();
}

//////////////////
/// + operator ///
//////////////////

template <typename T>
void plus_test()
{
  std::size_t       n            = 10000;
  ShapelessArray<T> test_array   = RandomArray(n, T(0));
  ShapelessArray<T> test_array_2 = RandomArray(n, T(0));
  ShapelessArray<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] + test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array + test_array_2));
}

TEST(ndarray, integer_plus_test)
{
  plus_test<int>();
}
TEST(ndarray, size_t_plus_test)
{
  plus_test<uint32_t>();
}
TEST(ndarray, float_plus_test)
{
  plus_test<float>();
}
TEST(ndarray, double_plus_test)
{
  plus_test<double>();
}

//////////////////
/// - operator ///
//////////////////

template <typename T>
void sub_test()
{
  std::size_t       n            = 10000;
  ShapelessArray<T> test_array   = RandomArray(n, T(0));
  ShapelessArray<T> test_array_2 = RandomArray(n, T(0));
  ShapelessArray<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] - test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array - test_array_2));
}

TEST(ndarray, integer_sub_test)
{
  sub_test<int>();
}
TEST(ndarray, size_t_sub_test)
{
  sub_test<uint32_t>();
}
TEST(ndarray, float_sub_test)
{
  sub_test<float>();
}
TEST(ndarray, double_sub_test)
{
  sub_test<double>();
}

//////////////////
/// * operator ///
//////////////////

template <typename T>
void mult_test()
{
  std::size_t       n            = 10000;
  ShapelessArray<T> test_array   = RandomArray(n, T(0));
  ShapelessArray<T> test_array_2 = RandomArray(n, T(0));
  ShapelessArray<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] * test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array * test_array_2));
}

TEST(ndarray, integer_mult_test)
{
  mult_test<int>();
}
TEST(ndarray, size_t_mult_test)
{
  mult_test<uint32_t>();
}
TEST(ndarray, float_mult_test)
{
  mult_test<float>();
}
TEST(ndarray, double_mult_test)
{
  mult_test<double>();
}

//////////////////
/// / operator ///
//////////////////

template <typename T>
void div_test()
{
  std::size_t       n            = 12;
  ShapelessArray<T> test_array   = RandomArray(n, T(1));
  ShapelessArray<T> test_array_2 = RandomArray(n, T(1));
  ShapelessArray<T> result_array = RandomArray(n, T(1));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] / test_array_2[j];
  }

  ShapelessArray<T> tmp(n);
  tmp = test_array / test_array_2;

  ASSERT_TRUE(result_array.AllClose(tmp));
}

TEST(ndarray, integer_div_test)
{
  div_test<int>();
}
TEST(ndarray, size_t_div_test)
{
  div_test<uint32_t>();
}
TEST(ndarray, float_div_test)
{
  div_test<float>();
}
TEST(ndarray, double_div_test)
{
  div_test<double>();
}

///////////////////////////
///// equality operator ///
///////////////////////////

template <typename T>
void is_equal_test()
{
  std::size_t       n          = 10000;
  ShapelessArray<T> test_array = RandomArray(n, T(0));
  ShapelessArray<T> test_array_2(n);
  test_array_2 = test_array.Copy();

  ASSERT_TRUE(test_array == test_array_2);
}

TEST(ndarray, integer_is_equal_test)
{
  is_equal_test<int>();
}
TEST(ndarray, size_t_is_equal_test)
{
  is_equal_test<uint32_t>();
}
TEST(ndarray, float_is_equal_test)
{
  is_equal_test<float>();
}
TEST(ndarray, double_is_equal_test)
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
  ShapelessArray<T> test_array = RandomArray(n, T(0));
  ShapelessArray<T> test_array_2(n);

  for (std::size_t j = 0; j < test_array.size(); ++j)
  {
    test_array_2.Set(j, test_array.At(j) + T(1));
  }

  ASSERT_TRUE(test_array != test_array_2);
}

TEST(ndarray, integer_is_not_equal_test)
{
  is_not_equal_test<int>();
}
TEST(ndarray, size_t_is_not_equal_test)
{
  is_not_equal_test<uint32_t>();
}
TEST(ndarray, float_is_not_equal_test)
{
  is_not_equal_test<float>();
}
TEST(ndarray, double_is_not_equal_test)
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
  ShapelessArray<T> test_array   = RandomArray(n, T(0));
  ShapelessArray<T> test_array_2 = test_array;
  ShapelessArray<T> result_array = test_array * T(2);

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    test_array_2[j] += test_array[j];
  }
  ASSERT_TRUE(test_array_2.AllClose(result_array));
}

TEST(ndarray, integer_plus_equals_test)
{
  plus_equals_test<int>();
}
TEST(ndarray, size_t_plus_equals_test)
{
  plus_equals_test<uint32_t>();
}
TEST(ndarray, float_plus_equals_test)
{
  plus_equals_test<float>();
}
TEST(ndarray, double_plus_equals_test)
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
  ShapelessArray<T> test_array   = RandomArray(n, T(0));
  ShapelessArray<T> test_array_2 = test_array * T(2);
  ShapelessArray<T> result_array = test_array;

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    test_array_2[j] -= test_array[j];
  }
  ASSERT_TRUE(test_array_2.AllClose(result_array));
}

TEST(ndarray, integer_minus_equals_test)
{
  minus_equals_test<int>();
}
TEST(ndarray, size_t_minus_equals_test)
{
  minus_equals_test<uint32_t>();
}
TEST(ndarray, float_minus_equals_test)
{
  minus_equals_test<float>();
}
TEST(ndarray, double_minus_equals_test)
{
  minus_equals_test<double>();
}
