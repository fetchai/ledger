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

#include "core/random/lcg.hpp"
#include "math/approx_exp.hpp"
#include "math/base_types.hpp"
#include "math/standard_functions/exp.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

template <uint8_t N, uint64_t C>
static void BM_ApproxExpImplementation(benchmark::State &state)
{

  fetch::math::ApproxExpImplementation<N, C> fexp;
  double                                     x      = 0.1;  //(double)state.range(0);
  double                                     result = 0;
  for (auto _ : state)
  {
    // Single iteration is too small to get accurate benchmarks.
    x += 0.1;
    for (int i = 0; i < 1000; i++)
    {
      x += 0.0001;
      result += fexp(x);
    }
  }
}
BENCHMARK_TEMPLATE(BM_ApproxExpImplementation, 0, 0)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_ApproxExpImplementation, 8, 60801)->RangeMultiplier(10)->Range(1, 100000);
BENCHMARK_TEMPLATE(BM_ApproxExpImplementation, 12, 60801)->RangeMultiplier(10)->Range(1, 1000000);

template <typename Type>
static void BM_exp(benchmark::State &state)
{
  Type x = fetch::math::Type<Type>("0.1");  // (double)state.range(0);
  Type result{0};
  for (auto _ : state)
  {
    // Single iteration is too small to get accurate benchmarks.
    for (int i = 0; i < 1000; i++)
    {
      result += fetch::math::Exp(x);
    }
  }
}

BENCHMARK_TEMPLATE(BM_exp, float)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_exp, double)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_exp, fetch::fixed_point::fp32_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_exp, fetch::fixed_point::fp64_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_exp, fetch::fixed_point::fp128_t)->RangeMultiplier(10)->Range(1, 1000000);

}  // namespace
