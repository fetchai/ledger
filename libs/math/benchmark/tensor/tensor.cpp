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

#include "math/tensor/tensor.hpp"

#include "benchmark/benchmark.h"

// size access should be very fast
template <class T, int C, int H, int W>
void BM_TensorSize(benchmark::State &state)
{
  fetch::math::Tensor<T> t(std::vector<uint64_t>{C, H, W});
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
  fetch::math::Tensor<T> t(std::vector<uint64_t>{C, H, W});
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
    fetch::math::Tensor<T> t(std::vector<uint64_t>{C, H, W});

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
    fetch::math::Tensor<T> t(std::vector<uint64_t>{C, H, W});

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
    fetch::math::Tensor<T> t(std::vector<uint64_t>{C, H, W});
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
  fetch::math::Tensor<T>              t1(std::vector<uint64_t>{C, H, W});
  fetch::math::Tensor<T>              t2(std::vector<uint64_t>{C, H, W});
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
void BM_TensorSlice(benchmark::State &state)
{
  fetch::math::Tensor<T> t(std::vector<uint64_t>{C, H, W});

  for (auto _ : state)
  {
    benchmark::DoNotOptimize(t.Slice(1, 1).begin());
  }
}

BENCHMARK_TEMPLATE(BM_TensorSlice, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSlice, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSlice, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorSlice, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSlice, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSlice, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_TensorSlice, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSlice, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_TensorSlice, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

struct BM_Tensor_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_Tensor_config(::benchmark::State const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(0));

    shape.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }
  }

  std::vector<SizeType> shape;  // layers input/output sizes
};

template <class T>
void BM_Iterate(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  // Get args form state
  BM_Tensor_config config{state};

  fetch::math::Tensor<T> input(config.shape);

  // Fill tensors with random values
  input.FillUniformRandom();

  state.counters["charge"] = static_cast<double>(TensorType::ChargeIterate(config.shape));

  for (auto _ : state)
  {
    auto it = input.begin();
    while (it.is_valid())
    {
      ++it;
    }
  }
}

static void AddArguments(benchmark::internal::Benchmark *b)
{
  using SizeType            = fetch::math::SizeType;
  SizeType const N_ELEMENTS = 2;
  std::int64_t   MAX_SIZE   = 2097152;

  std::vector<std::int64_t> dim_size;
  std::int64_t              i{1};
  while (i <= MAX_SIZE)
  {
    dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, j, 1});
  }
  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, 1, j});
  }
}

BENCHMARK_TEMPLATE(BM_Iterate, fetch::fixed_point::fp64_t)
    ->Apply(AddArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Iterate, float)->Apply(AddArguments)->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Iterate, double)->Apply(AddArguments)->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Iterate, fetch::fixed_point::fp32_t)
    ->Apply(AddArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Iterate, fetch::fixed_point::fp128_t)
    ->Apply(AddArguments)
    ->Unit(::benchmark::kNanosecond);

BENCHMARK_MAIN();
