//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/sign.hpp"
#include "math/tensor/tensor.hpp"

#include "gtest/gtest.h"

#include <cstddef>

namespace {

using namespace fetch::math;
using DataType      = double;
using ContainerType = fetch::memory::SharedArray<DataType>;

Tensor<DataType, ContainerType> RandomArray(std::size_t n, DataType adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  Tensor<DataType, ContainerType>                   a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = DataType(gen.AsDouble()) + adj;
  }
  return a1;
}

Tensor<DataType, ContainerType> ConstantArray(std::size_t n, DataType adj)
{
  Tensor<DataType, ContainerType> a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = adj;
  }
  return a1;
}

TEST(ndarray, zeros_out)
{
  std::size_t                     n            = 1000;
  Tensor<DataType, ContainerType> test_array   = ConstantArray(n, 0);
  Tensor<DataType, ContainerType> test_array_2 = RandomArray(n, -1.0);

  // sanity check that all values equal 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] == 0);
  }
}

TEST(ndarray, negative_ones)
{
  std::size_t                     n            = 1000;
  Tensor<DataType, ContainerType> test_array   = RandomArray(n, -1.0);
  Tensor<DataType, ContainerType> test_array_2 = RandomArray(n, 1.0);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] <= 0);
  }
}

TEST(ndarray, positive_ones)
{
  std::size_t                     n            = 1000;
  Tensor<DataType, ContainerType> test_array   = RandomArray(n, 1.0);
  Tensor<DataType, ContainerType> test_array_2 = RandomArray(n, -1.0);

  // sanity check that all values gt than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] >= 0);
  }
}

}  // namespace
