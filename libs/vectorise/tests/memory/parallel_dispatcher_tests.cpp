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

#include "vectorise/memory/shared_array.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

using namespace fetch::memory;

using type                 = float;
using ndarray_type         = SharedArray<type>;
using vector_register_type = typename ndarray_type::vector_register_type;
#define M 10000
#define N 100000

class ParallelDispatcherTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    a_ = ndarray_type(N);
    b_ = ndarray_type(N);
    for (std::size_t i = 0; i < N; ++i)
    {
      b_[i] = type(i);
    }
  }

  void TearDown() override
  {}

  ndarray_type a_, b_;
};
////////////
TEST_F(ParallelDispatcherTest, Comp_test)
{
  // Standard implementation
  for (std::size_t i = 0; i < M; ++i)
  {
    for (std::size_t j = 0; j < N; j += 4)
    {

      // We write it out such that the compiler might use SSE
      a_[j]     = std::exp(1 + std::log(b_[j]));
      a_[j + 1] = std::exp(1 + std::log(b_[j + 1]));
      a_[j + 2] = std::exp(1 + std::log(b_[j + 2]));
      a_[j + 3] = std::exp(1 + std::log(b_[j + 3]));
    }
  }
}

TEST_F(ParallelDispatcherTest, kernel_test)
{
  for (std::size_t i = 0; i < M; ++i)
  {

    // Here we use a_ kernel to compute the same, using an approximation
    a_.in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type &y) {
          static vector_register_type one(1);

          // We approximate the exponential function by a clever first order
          // Taylor expansion
          y = approx_exp(one + approx_log(x));
        },
        b_);
  }
}
