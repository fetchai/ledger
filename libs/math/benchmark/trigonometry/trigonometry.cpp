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

#include "math/trigonometry.hpp"
#include "core/random/lfg.hpp"
#include "math/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

template <class T>
void BM_Sin(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::Sin(val));
  }
}

BENCHMARK_TEMPLATE(BM_Sin, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Sin, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Sin, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Sin, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_Cos(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::Cos(val));
  }
}

BENCHMARK_TEMPLATE(BM_Cos, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Cos, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Cos, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Cos, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_Tan(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::Tan(val));
  }
}

BENCHMARK_TEMPLATE(BM_Tan, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Tan, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Tan, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Tan, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_ASin(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::ASin(val));
  }
}

BENCHMARK_TEMPLATE(BM_ASin, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ASin, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ASin, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ASin, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_ACos(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::ACos(val));
  }
}

BENCHMARK_TEMPLATE(BM_ACos, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ACos, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ACos, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ACos, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_ATan(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::ATan(val));
  }
}

BENCHMARK_TEMPLATE(BM_ATan, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ATan, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ATan, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ATan, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_SinH(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::SinH(val));
  }
}

BENCHMARK_TEMPLATE(BM_SinH, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SinH, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SinH, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SinH, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_CosH(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::CosH(val));
  }
}

BENCHMARK_TEMPLATE(BM_CosH, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CosH, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CosH, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CosH, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_TanH(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::TanH(val));
  }
}

BENCHMARK_TEMPLATE(BM_TanH, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanH, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanH, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanH, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_ASinH(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::ASinH(val));
  }
}

BENCHMARK_TEMPLATE(BM_ASinH, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ASinH, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ASinH, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ASinH, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_ACosH(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::ACosH(val));
  }
}

BENCHMARK_TEMPLATE(BM_ACosH, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ACosH, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ACosH, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ACosH, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

template <class T>
void BM_ATanH(benchmark::State &state)
{
  T                                         val;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  for (auto _ : state)
  {
    state.PauseTiming();
    val = T(lfg.AsDouble());
    state.ResumeTiming();
    benchmark::DoNotOptimize(fetch::math::ATanH(val));
  }
}

BENCHMARK_TEMPLATE(BM_ATanH, float)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ATanH, double)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ATanH, fetch::fixed_point::FixedPoint<16, 16>)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ATanH, fetch::fixed_point::FixedPoint<32, 32>)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
