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

#include "math/free_functions/free_functions.hpp"
#include "math/shapeless_array.hpp"
#include <gtest/gtest.h>

using namespace fetch::math;
using data_type      = double;
using container_type = fetch::memory::SharedArray<data_type>;

ShapelessArray<data_type, container_type> RandomArray(std::size_t n, data_type adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ShapelessArray<data_type, container_type>         a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = data_type(gen.AsDouble()) + adj;
  }
  return a1;
}
ShapelessArray<data_type, container_type> ConstantArray(std::size_t n, data_type adj)
{
  ShapelessArray<data_type, container_type> a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = adj;
  }
  return a1;
}

TEST(ndarray, zeros_out)
{
  std::size_t                               n            = 1000;
  ShapelessArray<data_type, container_type> test_array   = ConstantArray(n, 0);
  ShapelessArray<data_type, container_type> test_array_2 = RandomArray(n, -1.0);

  // sanity check that all values equal 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] == 0);
  }

  // check that sign(0) = 0
  /*    
  // TODO
  test_array_2 = test_array;
  fetch::math::Sign(test_array_2);

  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array_2[i] == 0);
  }
  */
}

TEST(ndarray, negative_ones)
{
  std::size_t                               n            = 1000;
  ShapelessArray<data_type, container_type> test_array   = RandomArray(n, -1.0);
  ShapelessArray<data_type, container_type> test_array_2 = RandomArray(n, 1.0);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] <= 0);
  }

  // check that sign(-) = -1
  /*
  // TODO
  test_array_2 = test_array;
  fetch::math::Sign(test_array_2);

  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array_2[i] == -1);
  }
  */
}

TEST(ndarray, positive_ones)
{
  std::size_t                               n            = 1000;
  ShapelessArray<data_type, container_type> test_array   = RandomArray(n, 1.0);
  ShapelessArray<data_type, container_type> test_array_2 = RandomArray(n, -1.0);

  // sanity check that all values gt than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] >= 0);
  }

  /*
  // TODO
  test_array_2 = test_array;
  fetch::math::Sign(test_array_2);

  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array_2[i] == 1);
  }
  */
}
