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

#include "benchmark/benchmark.h"
#include "math/base_types.hpp"
#include "math/standard_functions/log.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace {

template <typename Type>
static void BM_Log(benchmark::State &state)
{
  Type x      = fetch::math::Type<Type>("3");
  Type result = fetch::math::Type<Type>("0");
  for (auto _ : state)
  {
    // Single iteration is too small to get accurate benchmarks.
    for (int i = 0; i < 1000; i++)
    {
      fetch::math::Log(x, result);
      state.PauseTiming();
      x += fetch::math::Type<Type>("0.00001");
      state.ResumeTiming();
    }
  }
}

BENCHMARK_TEMPLATE(BM_Log, float)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log, double)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log, fetch::fixed_point::fp32_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log, fetch::fixed_point::fp64_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log, fetch::fixed_point::fp128_t)->RangeMultiplier(10)->Range(1, 1000000);

template <typename Type>
static void BM_Log2(benchmark::State &state)
{
  Type x      = fetch::math::Type<Type>("1");
  Type result = fetch::math::Type<Type>("0");
  for (auto _ : state)
  {
    // Single iteration is too small to get accurate benchmarks.
    for (int i = 0; i < 1000; i++)
    {
      fetch::math::Log2(x, result);
      state.PauseTiming();
      x += fetch::math::Type<Type>("0.00001");
      state.ResumeTiming();
    }
  }
}

BENCHMARK_TEMPLATE(BM_Log2, float)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log2, double)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log2, fetch::fixed_point::fp32_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log2, fetch::fixed_point::fp64_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log2, fetch::fixed_point::fp128_t)->RangeMultiplier(10)->Range(1, 1000000);

template <typename Type>
static void BM_Log10(benchmark::State &state)
{
  Type x      = fetch::math::Type<Type>("1.1245321");
  Type result = fetch::math::Type<Type>("0");
  for (auto _ : state)
  {
    // Single iteration is too small to get accurate benchmarks.
    for (int i = 0; i < 1000; i++)
    {
      fetch::math::Log10(x, result);
      state.PauseTiming();
      x += fetch::math::Type<Type>("0.00001");
      state.ResumeTiming();
    }
  }
}

BENCHMARK_TEMPLATE(BM_Log10, float)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log10, double)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log10, fetch::fixed_point::fp32_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log10, fetch::fixed_point::fp64_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Log10, fetch::fixed_point::fp128_t)->RangeMultiplier(10)->Range(1, 1000000);

}  // namespace
