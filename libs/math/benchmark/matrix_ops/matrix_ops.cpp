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

#include <iomanip>
#include <iostream>

#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"

#include "benchmark/benchmark.h"

template <class T, int C, int H, int W>
void BM_BooleanMaskEmpty(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;

  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<SizeType>{C, H, W});
    fetch::math::Tensor<T> mask(std::vector<SizeType>{C, H, W});
    mask.SetAllZero();
    state.ResumeTiming();

    fetch::math::BooleanMask(t, mask);
  }
}

BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_BooleanMaskFull(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<SizeType>{C, H, W});
    fetch::math::Tensor<T> mask(std::vector<SizeType>{C, H, W});
    mask.SetAllOne();
    state.ResumeTiming();
    fetch::math::BooleanMask(t, mask);
  }
}

BENCHMARK_TEMPLATE(BM_BooleanMaskFull, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskFull, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskFull, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_Max(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    state.ResumeTiming();
    fetch::math::Max(t);
  }
}

BENCHMARK_TEMPLATE(BM_Max, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Max, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Max, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_MaxAxis(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    fetch::math::Tensor<T> ret(std::vector<std::uint64_t>{C, W});
    state.ResumeTiming();
    fetch::math::Max(t, SizeType(1), ret);
  }
}

BENCHMARK_TEMPLATE(BM_MaxAxis, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MaxAxis, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MaxAxis, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
