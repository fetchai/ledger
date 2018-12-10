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
#include "math/shape_less_array.hpp"
#include <gtest/gtest.h>

using namespace fetch::math;

template <typename T>
ShapeLessArray<T, fetch::memory::SharedArray<T>> RandomArray(std::size_t n, T adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ShapeLessArray<T, fetch::memory::SharedArray<T>>         a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = T(gen.AsDouble()) + adj;
  }
  return a1;
}

///////////////////////
/// equals operator ///
///////////////////////

template <typename T>
void equal_test()
{
  std::size_t n = 10000;
  ShapeLessArray<T> test_array       = RandomArray<T>(n, 0);
  ShapeLessArray<T> result_array     = test_array;

  ASSERT_TRUE(result_array.AllClose(test_array));
}

TEST(ndarray, int_equals_test){equal_test<int>();}
TEST(ndarray, size_t_equals_test){equal_test<std::size_t>();}
TEST(ndarray, float_equals_test){equal_test<float>();}
TEST(ndarray, double_equals_test){equal_test<double>();}


////////////
/// copy ///
////////////

template <typename T>
void copy_test()
{
  std::size_t n = 10000;
  ShapeLessArray<T> test_array       = RandomArray<T>(n, 0);
  ShapeLessArray<T> result_array(n);
  result_array.Copy(test_array);

  ASSERT_TRUE(result_array.AllClose(test_array));
}

TEST(ndarray, int_copy_test){equal_test<int>();}
TEST(ndarray, size_t_copy_test){equal_test<std::size_t>();}
TEST(ndarray, float_copy_test){equal_test<float>();}
TEST(ndarray, double_copy_test){equal_test<double>();}


//////////////////
/// + operator ///
//////////////////

template <typename T>
void plus_test()
{
  std::size_t n = 10000;
  ShapeLessArray<T> test_array = RandomArray(n, T(0));
  ShapeLessArray<T> test_array_2 = RandomArray(n, T(0));
  ShapeLessArray<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] + test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array + test_array_2));
}

TEST(ndarray, integer_plus_test){plus_test<int>();}
//TEST(ndarray, size_t_plus_test){plus_test<std::size_t>();}
TEST(ndarray, float_plus_test){plus_test<float>();}
TEST(ndarray, double_plus_test){plus_test<double>();}


//////////////////
/// - operator ///
//////////////////

template <typename T>
void sub_test()
{
  std::size_t n = 10000;
  ShapeLessArray<T> test_array = RandomArray(n, T(0));
  ShapeLessArray<T> test_array_2 = RandomArray(n, T(0));
  ShapeLessArray<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] - test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array - test_array_2));
}

TEST(ndarray, integer_sub_test){sub_test<int>();}
//TEST(ndarray, size_t_sub_test){sub_test<std::size_t>();}
TEST(ndarray, float_sub_test){sub_test<float>();}
TEST(ndarray, double_sub_test){sub_test<double>();}


//////////////////
/// * operator ///
//////////////////

template <typename T>
void mult_test()
{
  std::size_t n = 10000;
  ShapeLessArray<T> test_array = RandomArray(n, T(0));
  ShapeLessArray<T> test_array_2 = RandomArray(n, T(0));
  ShapeLessArray<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] * test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array * test_array_2));
}

TEST(ndarray, integer_mult_test){mult_test<int>();}
//TEST(ndarray, size_t_mult_test){mult_test<std::size_t>();}
TEST(ndarray, float_mult_test){mult_test<float>();}
TEST(ndarray, double_mult_test){mult_test<double>();}


//////////////////
/// / operator ///
//////////////////

template <typename T>
void div_test()
{
  std::size_t n = 10000;
  ShapeLessArray<T> test_array = RandomArray(n, T(0));
  ShapeLessArray<T> test_array_2 = RandomArray(n, T(0));
  ShapeLessArray<T> result_array = RandomArray(n, T(0));

  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] / test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array / test_array_2));
}

//TEST(ndarray, integer_div_test){div_test<int>();}
//TEST(ndarray, size_t_div_test){div_test<std::size_t>();}
TEST(ndarray, float_div_test){div_test<float>();}
TEST(ndarray, double_div_test){div_test<double>();}



//
/////////////////////////////
///// not-equals operator ///
/////////////////////////////
//
//TEST(ndarray, int_equals_test)
//{
//  using data_type = int;
//  using container_type = fetch::memory::SharedArray<data_type>;
//  std::size_t                               n                = 10000;
//  ShapeLessArray<data_type, container_type> test_array       = RandomArray<data_type>(n, 0);
//  ShapeLessArray<data_type, container_type> result_array     = test_array;
//
//  ASSERT_TRUE(result_array.AllClose(test_array));
//}
//
//TEST(ndarray, size_t_equals_test)
//{
//  using data_type = std::size_t;
//  using container_type = fetch::memory::SharedArray<data_type>;
//  std::size_t                               n                = 10000;
//  ShapeLessArray<data_type, container_type> test_array       = RandomArray<data_type>(n, 0);
//  ShapeLessArray<data_type, container_type> result_array     = test_array;
//
//  ASSERT_TRUE(result_array.AllClose(test_array));
//}
//
//TEST(ndarray, double_equals_test)
//{
//  using data_type = double;
//  using container_type = fetch::memory::SharedArray<data_type>;
//  std::size_t                               n                = 10000;
//  ShapeLessArray<data_type, container_type> test_array       = RandomArray<data_type>(n, 0);
//  ShapeLessArray<data_type, container_type> result_array     = test_array;
//
//  ASSERT_TRUE(result_array.AllClose(test_array));
//}
//
//TEST(ndarray, float_equals_test)
//{
//  using data_type = float;
//  using container_type = fetch::memory::SharedArray<data_type>;
//  std::size_t                               n                = 10000;
//  ShapeLessArray<data_type, container_type> test_array       = RandomArray<data_type>(n, 0);
//  ShapeLessArray<data_type, container_type> result_array     = test_array;
//
//  ASSERT_TRUE(result_array.AllClose(test_array));
//}
//
//















