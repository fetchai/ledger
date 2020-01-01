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

#include "math/base_types.hpp"
#include "math/standard_functions/abs.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

namespace {

template <typename DataType>
static void BM_Abs(benchmark::State &state)
{
  DataType x = fetch::math::Type<DataType>("1");

  for (auto _ : state)
  {
    state.PauseTiming();
    x = DataType{2} * -x;
    state.ResumeTiming();
    fetch::math::Abs(x, x);
  }
}

BENCHMARK_TEMPLATE(BM_Abs, int32_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Abs, int64_t)->RangeMultiplier(10)->Range(1, 1000000);

BENCHMARK_TEMPLATE(BM_Abs, float)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Abs, double)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Abs, fetch::fixed_point::fp32_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Abs, fetch::fixed_point::fp64_t)->RangeMultiplier(10)->Range(1, 1000000);
BENCHMARK_TEMPLATE(BM_Abs, fetch::fixed_point::fp128_t)->RangeMultiplier(10)->Range(1, 1000000);

}  // namespace
