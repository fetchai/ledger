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

using data_type = int;
using container_type = fetch::memory::SharedArray<data_type>;

ShapeLessArray<data_type, container_type> RandomArray(std::size_t n, data_type adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ShapeLessArray<data_type, container_type>         a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = data_type(gen.AsDouble()) + adj;
  }
  return a1;
}

TEST(ndarray, integer_equals_test)
{
  std::size_t                               n                = 10000;
  ShapeLessArray<data_type, container_type> test_array       = RandomArray(n, 0);
  ShapeLessArray<data_type, container_type> result_array     = test_array;

  ASSERT_TRUE(result_array.AllClose(test_array));
}

TEST(ndarray, integer_copy_test)
{
  std::size_t                               n                = 10000;
  ShapeLessArray<data_type, container_type> test_array       = RandomArray(n, 0);
  ShapeLessArray<data_type, container_type> result_array(n);
  result_array.Copy(test_array);

  ASSERT_TRUE(result_array.AllClose(test_array));
}


TEST(ndarray, integer_basic_operator_test_1)
{
  std::size_t n = 10000;
  ShapeLessArray<data_type, container_type> test_array = RandomArray(n, 0);
  ShapeLessArray<data_type, container_type> test_array_2 = RandomArray(n, 0);
  ShapeLessArray<data_type, container_type> result_array = RandomArray(n, 0);

  /**
   * operator +
   */
  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] + test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array + test_array_2));

  /**
   * operator -
   */
  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] - test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array - test_array_2));
}


TEST(ndarray, integer_basic_operator_test_2)
{
  std::size_t n = 10000;
  ShapeLessArray<data_type, container_type> test_array = RandomArray(n, 0);
  ShapeLessArray<data_type, container_type> test_array_2 = RandomArray(n, 0);
  ShapeLessArray<data_type, container_type> result_array = RandomArray(n, 0);

  /**
   * operator *
   */
  for (std::size_t j = 0; j < result_array.size(); ++j)
  {
    result_array[j] = test_array[j] * test_array_2[j];
  }
  ASSERT_TRUE(result_array.AllClose(test_array * test_array_2));
//
//  /**
//   * operator /
//   */
//  for (std::size_t j = 0; j < result_array.size(); ++j)
//  {
//    result_array[j] = test_array[j] / test_array_2[j];
//  }
//  ASSERT_TRUE(result_array.AllClose(test_array / test_array_2));

}
