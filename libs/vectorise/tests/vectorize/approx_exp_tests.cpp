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

#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <iostream>

using type        = float;
using array_type  = fetch::memory::SharedArray<type>;
using vector_type = typename array_type::vector_register_type;

void Exponentials(array_type const &A, array_type &C)
{
  C.in_parallel().Apply(
      [](vector_type const &x, vector_type &y) { y = fetch::vectorize::approx_exp(x); }, A);
}

TEST(vectorise_approx_exp_gtest, exp_test)
{
  std::size_t N       = std::size_t(20);
  float       exp_val = 0.0, initial_value = 1;
  array_type  A(N), C(N);

  for (std::size_t j = 1; j < 12; j++)
  {
    initial_value = initial_value * 2;
    std::cout << initial_value << "\n";
    A[0] = float(initial_value - 1.0);
    for (std::size_t i = 1; i < N; ++i)
    {
      A[i] = float(A[i - 1] + 0.1);
    }
    Exponentials(A, C);
    for (std::size_t i = 0; i < N; ++i)
    {
      exp_val = std::exp(A[i]);
      std::cout << "values : Actual value = " << A[i] << "  fetch_approx_exp = " << C[i]
                << "  std::exp =" << exp_val << " diff is =" << exp_val - C[i] << "\n";
    }
  }
}