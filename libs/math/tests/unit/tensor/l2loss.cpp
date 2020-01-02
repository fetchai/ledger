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

#include "math/metrics/l2_loss.hpp"
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

TEST(ndarray, l2_basic)
{
  double epsilon = 1e-12;

  std::size_t                     n                = 10000;
  Tensor<DataType, ContainerType> test_array       = RandomArray(n, -0.5);
  double                          test_loss        = 0;
  double                          manual_test_loss = 0;

  // check that sign(0) = 0
  test_loss = fetch::math::L2Loss(test_array);

  for (std::size_t i = 0; i < n; ++i)
  {
    manual_test_loss += (test_array[i] * test_array[i]);
  }
  manual_test_loss /= 2;

  ASSERT_TRUE(manual_test_loss - test_loss < epsilon);
}

}  // namespace
