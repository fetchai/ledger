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
#include <chrono>
#include <cmath>
#include <iostream>

#include "core/random/lcg.hpp"
#include "math/approx_exp.hpp"



template <uint8_t N, uint64_t C>
static void BM_ApproxExpImplementation(benchmark::State &state)
{

  fetch::math::ApproxExpImplementation<N, C> fexp;
  double x = state.range(0);
  for (auto _ : state){
    //Running loop to get greater benchmarks.  
    for(int i=0; i<1000; i++){
       x = fexp(x);
    }
  }
}
BENCHMARK_TEMPLATE(BM_ApproxExpImplementation, 0,0)->RangeMultiplier(10)->Range(1, 1000000)->Complexity();
BENCHMARK_TEMPLATE(BM_ApproxExpImplementation, 8,60801)->RangeMultiplier(10)->Range(1, 1000000)->Complexity();
BENCHMARK_TEMPLATE(BM_ApproxExpImplementation, 12,60801)->RangeMultiplier(10)->Range(1, 1000000)->Complexity();


static void BM_exp(benchmark::State &state)
{
  double x = state.range(0);
  for (auto _ : state){
    //Running loop to get greater benchmarks. 
   for(int i=0; i<1000; i++){
       x = exp(x);
    }
  }
}
BENCHMARK(BM_exp)->RangeMultiplier(10)->Range(1, 1000000)->Complexity();


BENCHMARK_MAIN();