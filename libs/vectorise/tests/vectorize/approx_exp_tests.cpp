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

#include "vectorise/math/exp.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <iostream>

using type        = double;
using array_type  = fetch::memory::Array<type>;
using vector_type = typename array_type::vector_register_type;

void Exponentials(array_type const &A, array_type &C)
{
  C.in_parallel().Apply([](vector_type const &a, vector_type &c) { c = fetch::vectorize::exp(a); },
                        A);
}

TEST(vectorise_approx_exp_gtest, basic_test_with_array_size_100)
{

  std::size_t N = std::size_t(100);
  array_type  A(N), C(N);

  for (std::size_t i = 0; i < N; ++i)
  {
    A[i] = double(0.1 * type(i) - type(double(N) * 0.5));
  }

  Exponentials(A, C);
  for (std::size_t i = 0; i < N; ++i)
  {
    std::cout << A[i] << " " << C[i] << " " << std::exp(A[i]) << std::endl;
  }
}