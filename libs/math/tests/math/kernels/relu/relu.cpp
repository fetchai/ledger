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
#include "math/kernels/relu.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

using namespace fetch::math;
using DataType       = double;
using container_type = fetch::memory::SharedArray<DataType>;

Tensor<DataType> RandomArrayNegative(std::size_t n, std::size_t /*m*/)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<DataType>                                  a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = DataType(gen.AsDouble()) - 1.0;
  }
  return a1;
}

Tensor<DataType> RandomArrayPositive(std::size_t n, std::size_t /*m*/)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<DataType>                                  a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = DataType(gen.AsDouble());
  }
  return a1;
}

TEST(ndarray, zeros_out)
{

  std::size_t      n            = 1000;
  Tensor<DataType> test_array   = RandomArrayNegative(n, n);
  Tensor<DataType> test_array_2 = RandomArrayNegative(n, n);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] < 0);
  }

  //
  fetch::math::Relu(test_array, test_array);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] == DataType(0));
  }
}

TEST(ndarray, linear_response)
{

  std::size_t      n            = 1000;
  Tensor<DataType> test_array   = RandomArrayPositive(n, n);
  Tensor<DataType> test_array_2 = RandomArrayPositive(n, n);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] >= 0);
  }

  test_array_2 = test_array;
  fetch::math::Relu(test_array_2);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array_2[i] == test_array[i]);
  }
}
