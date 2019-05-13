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

#include "benchmark/benchmark.h"

// size access should be very fast
template <class T, int C, int H, int W>
void BM_TensorSize(benchmark::State &state)
{
  fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
  for (auto _ : state)
  {
    benchmark::DoNotOptimize(t.size());
  }
}

BENCHMARK_TEMPLATE(BM_TensorSize, int, 3, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorSize, float, 3, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorSize, double, 3, 256, 256)->Unit(benchmark::kNanosecond);

BENCHMARK_TEMPLATE(BM_TensorSize, int, 128, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorSize, float, 128, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorSize, double, 128, 256, 256)->Unit(benchmark::kNanosecond);

BENCHMARK_TEMPLATE(BM_TensorSize, int, 256, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorSize, float, 256, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorSize, double, 256, 256, 256)->Unit(benchmark::kNanosecond);

// shape access should also be very fast
template <class T, int C, int H, int W>
void BM_TensorShape(benchmark::State &state)
{
  fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
  for (auto _ : state)
  {
    benchmark::DoNotOptimize(t.shape());
  }
}

BENCHMARK_TEMPLATE(BM_TensorShape, int, 3, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorShape, float, 3, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorShape, double, 3, 256, 256)->Unit(benchmark::kNanosecond);

BENCHMARK_TEMPLATE(BM_TensorShape, int, 128, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorShape, float, 128, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorShape, double, 128, 256, 256)->Unit(benchmark::kNanosecond);

BENCHMARK_TEMPLATE(BM_TensorShape, int, 256, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorShape, float, 256, 256, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorShape, double, 256, 256, 256)->Unit(benchmark::kNanosecond);

template <class T, int C, int H, int W>
void BM_TensorNaiveIteration(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();

    // Construct reference tensor
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});

    state.ResumeTiming();

    for (auto const &e : t)
    {
      benchmark::DoNotOptimize(e);
    }
  }
}

BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorNaiveIteration, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

// Reference implementation of a vector to compare iteration time
template <class T, int C, int H, int W>
void VectorBaselineRangeIterator(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();

    // Construct reference tensor
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});

    // Baseline - iterate over vector of same number of elements
    std::vector<T> baseline;
    baseline.resize(t.size());
    state.ResumeTiming();

    for (auto const &e : baseline)
    {
      benchmark::DoNotOptimize(e);
    }
  }
}

BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, float, 128, 256, 256)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, double, 128, 256, 256)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, float, 256, 256, 256)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(VectorBaselineRangeIterator, double, 256, 256, 256)
    ->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_TensorRangeIterator(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    state.ResumeTiming();

    for (auto const &e : t)
    {
      benchmark::DoNotOptimize(e);
    }
  }
}

BENCHMARK_TEMPLATE(BM_TensorRangeIterator, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorRangeIterator, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorRangeIterator, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorRangeIterator, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorRangeIterator, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorRangeIterator, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorRangeIterator, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorRangeIterator, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorRangeIterator, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_TensorConcat(benchmark::State &state)
{
  fetch::math::Tensor<T>              t1(std::vector<std::uint64_t>{C, H, W});
  fetch::math::Tensor<T>              t2(std::vector<std::uint64_t>{C, H, W});
  std::vector<fetch::math::Tensor<T>> vt{t1, t2};

  for (auto _ : state)
  {
    benchmark::DoNotOptimize(fetch::math::Tensor<T>::Concat(vt, 0));
  }
}

BENCHMARK_TEMPLATE(BM_TensorConcat, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorConcat, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorConcat, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorConcat, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorConcat, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorConcat, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorConcat, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorConcat, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorConcat, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_TensorSum(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    state.ResumeTiming();

    benchmark::DoNotOptimize(t.Sum());
  }
}

BENCHMARK_TEMPLATE(BM_TensorSum, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSum, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSum, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorSum, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSum, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSum, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorSum, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSum, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSum, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
