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
#include "math/spline/linear.hpp"

// This is to avoid ambiguity in instantation
double dsin(double x)
{
  return sin(x);
}
double dcos(double x)
{
  return cos(x);
}
double dtan(double x)
{
  return tan(x);
}
double dexp(double x)
{
  return exp(x);
}


fetch::math::spline::Spline<> spline;


static void CustomArguments(benchmark::internal::Benchmark* b) {
  for (int i = 0; i <= 720;i=i+180 ){
    b->Arg(i);
  }
}




template <std::size_t N>
static void BM_sin_spline(benchmark::State &state)
{
  spline.SetFunction(dsin, 0, 100, N);
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = spline(x);
  }
}
BENCHMARK_TEMPLATE(BM_sin_spline, 8)->Apply(CustomArguments)->Complexity();
BENCHMARK_TEMPLATE(BM_sin_spline, 16)->RangeMultiplier(10)->Range(1, 100)->Complexity();
BENCHMARK_TEMPLATE(BM_sin_spline, 20)->RangeMultiplier(10)->Range(1, 100)->Complexity();


static void BM_sin(benchmark::State &state)
{
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = sin(x);
  }
}
BENCHMARK(BM_sin)->Apply(CustomArguments)->Complexity();


template <std::size_t N>
static void BM_cos_spline(benchmark::State &state)
{
  spline.SetFunction(dcos, 0, 100, N);
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = spline(x);
  }
}
BENCHMARK_TEMPLATE(BM_cos_spline, 8)->Apply(CustomArguments)->Complexity();
BENCHMARK_TEMPLATE(BM_cos_spline, 16)->Apply(CustomArguments)->Complexity();
BENCHMARK_TEMPLATE(BM_cos_spline, 20)->RangeMultiplier(10)->Range(1, 100)->Complexity();


static void BM_cos(benchmark::State &state)
{
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = cos(x);
  }
}
BENCHMARK(BM_cos)->Apply(CustomArguments)->Complexity();

template <std::size_t N>
static void BM_tan_spline(benchmark::State &state)
{
  spline.SetFunction(dtan, 0, 100, N);
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = spline(x);
  }
}
BENCHMARK_TEMPLATE(BM_tan_spline, 8)->Apply(CustomArguments)->Complexity();
BENCHMARK_TEMPLATE(BM_tan_spline, 16)->Apply(CustomArguments)->Complexity();
BENCHMARK_TEMPLATE(BM_tan_spline, 20)->RangeMultiplier(10)->Range(1, 100)->Complexity();;


static void BM_tan(benchmark::State &state)
{
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = tan(x);
  }
}
BENCHMARK(BM_tan)->Apply(CustomArguments)->Complexity();


template <std::size_t N>
static void BM_exp_spline(benchmark::State &state)
{
  spline.SetFunction(dexp, 0, 100, N);
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = spline(x);
  }
}
BENCHMARK_TEMPLATE(BM_exp_spline, 8)->Apply(CustomArguments)->Complexity();
BENCHMARK_TEMPLATE(BM_exp_spline, 16)->Apply(CustomArguments)->Complexity();
BENCHMARK_TEMPLATE(BM_exp_spline, 20)->RangeMultiplier(10)->Range(1, 100)->Complexity();


static void BM_exponent(benchmark::State &state)
{
  volatile double x;
  volatile double y;
  for (auto _ : state)
  {
    x = state.range(0);
    y = exp(x);
  }
}
BENCHMARK(BM_exponent)->Apply(CustomArguments)->Complexity();
