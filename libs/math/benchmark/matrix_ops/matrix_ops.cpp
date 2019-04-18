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

#include "math/tensor.hpp"
#include "math/matrix_operations.hpp"

#include "benchmark/benchmark.h"

template <class T, int C, int H, int W>
void BM_BooleanMaskEmpty(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    fetch::math::Tensor<T> mask(std::vector<std::uint64_t>{C, H, W});
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
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    fetch::math::Tensor<T> mask(std::vector<std::uint64_t>{C, H, W});
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




//
//// Reference implementation of a vector to compare iteration time
//template <class T, int C, int H, int W>
//void VectorBaselineRangeIterator(benchmark::State &state)
//{
//  for (auto _ : state)
//  {
//    state.PauseTiming();
//
//    // Construct reference tensor
//    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
//
//    // Baseline - iterate over vector of same number of elements
//    std::vector<T> baseline;
//    baseline.resize(t.size());
//    state.ResumeTiming();
//
//    for (auto const &e : baseline)
//    {
//      benchmark::DoNotOptimize(e);
//    }
//  }
//}
//
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, double, 3, 256, 256)->Unit(benchmark::kMillisecond);
//
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, float, 128, 256, 256)
//    ->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, double, 128, 256, 256)
//    ->Unit(benchmark::kMillisecond);
//
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, float, 256, 256, 256)
//    ->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, double, 256, 256, 256)
//    ->Unit(benchmark::kMillisecond);
//
//template <class T, int C, int H, int W>
//void BM_tensorRangeIterator(benchmark::State &state)
//{
//  for (auto _ : state)
//  {
//    state.PauseTiming();
//    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
//    state.ResumeTiming();
//
//    for (auto const &e : t)
//    {
//      benchmark::DoNotOptimize(e);
//    }
//  }
//}
//
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, double, 3, 256, 256)->Unit(benchmark::kMillisecond);
//
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, double, 128, 256, 256)->Unit(benchmark::kMillisecond);
//
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorRangeIterator, double, 256, 256, 256)->Unit(benchmark::kMillisecond);
//
//template <class T, int C, int H, int W>
//void BM_tensorSum(benchmark::State &state)
//{
//  for (auto _ : state)
//  {
//    state.PauseTiming();
//    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
//    state.ResumeTiming();
//
//    benchmark::DoNotOptimize(t.Sum());
//  }
//}
//
//BENCHMARK_TEMPLATE(BM_tensorSum, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorSum, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorSum, double, 3, 256, 256)->Unit(benchmark::kMillisecond);
//
//BENCHMARK_TEMPLATE(BM_tensorSum, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorSum, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorSum, double, 128, 256, 256)->Unit(benchmark::kMillisecond);
//
//BENCHMARK_TEMPLATE(BM_tensorSum, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorSum, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
//BENCHMARK_TEMPLATE(BM_tensorSum, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
