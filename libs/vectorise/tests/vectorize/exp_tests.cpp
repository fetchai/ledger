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

#include "vectorise/math/exp.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"

#include "gtest/gtest.h"

#include <cmath>
#include <cstddef>

using type        = double;
using array_type  = fetch::memory::Array<type>;
using vector_type = typename array_type::VectorRegisterType;

void Exponentials(array_type const &A, array_type &C)
{
  C.in_parallel().Apply([](vector_type const &a, vector_type &c) { c = fetch::vectorize::exp(a); },
                        A);
}

TEST(vectorise_exp_gtest, exp_test)
{
  std::size_t N = 100u;
  array_type  A(N), C(N);
  // Exponent value not accurate outside these bounds(-5 , 5) due to being a taylor series around 0
  A[0] = double(-5);
  for (std::size_t i = 1; i < N; ++i)
  {
    A[i] = A[i - 1] + 0.1;
  }

  Exponentials(A, C);
  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_NEAR(C[i], std::exp(A[i]), 0.0001);
  }
}