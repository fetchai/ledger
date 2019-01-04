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

#include "benchmark/benchmark.h"
#include "math/shapeless_array.hpp"

using namespace fetch::math;

using Type      = double;
using ArrayType = fetch::math::ShapelessArray<Type>;

bool shapeless_array_equality_equal(std::size_t size_1, std::size_t size_2, int diff_element = -1)
{
  ArrayType array_1 = ArrayType::Zeroes(size_1);
  ArrayType array_2 = ArrayType::Zeroes(size_2);

  if (diff_element == -1)
  {
    return array_1 == array_2;
  }
  else
  {
    array_1[diff_element] = 1;
    return array_1 == array_2;
  }
}

static void BM_shapeless_array_equality_equal_small(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(10, 10);
  }
}
BENCHMARK(BM_shapeless_array_equality_equal_small);
static void BM_shapeless_array_equality_equal_medium(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(1000, 1000);
  }
}
BENCHMARK(BM_shapeless_array_equality_equal_medium);
static void BM_shapeless_array_equality_equal_large(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(100000000, 100000000);
  }
}
BENCHMARK(BM_shapeless_array_equality_equal_large);

static void BM_shapeless_array_equality_unequal_start_small(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(10, 10, 1);
  }
}
BENCHMARK(BM_shapeless_array_equality_unequal_start_small);
static void BM_shapeless_array_equality_unequal_start_medium(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(1000, 1000, 10);
  }
}
BENCHMARK(BM_shapeless_array_equality_unequal_start_medium);
static void BM_shapeless_array_equality_unequal_start_large(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(100000000, 100000000, 100);
  }
}
BENCHMARK(BM_shapeless_array_equality_unequal_start_large);

static void BM_shapeless_array_equality_unequal_end_small(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(10, 10, 9);
  }
}
BENCHMARK(BM_shapeless_array_equality_unequal_end_small);
static void BM_shapeless_array_equality_unequal_end_medium(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(1000, 1000, 999);
  }
}
BENCHMARK(BM_shapeless_array_equality_unequal_end_medium);
static void BM_shapeless_array_equality_unequal_end_large(benchmark::State &state)
{
  for (auto _ : state)
  {
    shapeless_array_equality_equal(100000000, 100000000, 99999999);
  }
}
BENCHMARK(BM_shapeless_array_equality_unequal_end_large);

BENCHMARK_MAIN();
