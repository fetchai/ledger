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

#include <cstdint>
#include <iostream>
#include <vector>

#include "core/random/lcg.hpp"
#include "math/rectangular_array.hpp"

#include <gtest/gtest.h>

using namespace fetch::math;

using data_type  = double;
using array_type = RectangularArray<data_type>;

TEST(shared_rectangulary_array_gtest, basic_test)
{
  std::vector<double>                               dataset;
  static fetch::random::LinearCongruentialGenerator gen;
  std::size_t                                       N = gen() % 5000;
  std::size_t                                       M = gen() % 5000;

  array_type mem(N, M);

  ASSERT_FALSE((N != mem.height()) || (M != mem.width())) << "size mismatch I ";

  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      data_type d = gen.AsDouble();
      mem(i, j)   = d;
      dataset.push_back(d);
    }
  }

  std::size_t k = 0;
  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      ASSERT_EQ(mem(i, j), dataset[k]) << "Data differs!";
      ++k;
    }
  }

  for (std::size_t i = 0; i < N * M; ++i)
  {
    ASSERT_EQ(mem[i], dataset[i]) << "Data differs II!";
  }

  array_type mem2(mem);
  ASSERT_FALSE((mem2.height() != mem.height()) || (mem2.width() != mem.width()))
      << "size mismatch II ";

  k = 0;
  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      ASSERT_EQ(mem2(i, j), dataset[k]) << "Data differs III!";
      ++k;
    }
  }

  for (std::size_t i = 0; i < N * M; ++i)
  {
    ASSERT_EQ(mem2[i], dataset[i]) << "Data differs IV!";
  }

  array_type mem3;
  mem3 = mem;
  ASSERT_FALSE((mem3.height() != mem.height()) || (mem3.width() != mem.width()))
      << "size mismatch III ";

  k = 0;
  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      ASSERT_EQ(mem3(i, j), dataset[k]) << "Data differs V!";
      ++k;
    }
  }

  for (std::size_t i = 0; i < N * M; ++i)
  {
    ASSERT_EQ(mem3[i], dataset[i]) << "Data differs VI!";
  }
}