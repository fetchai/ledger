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
#include "ml/dataloaders/tensor_dataloader.hpp"

#include "benchmark/benchmark.h"

#include <string>

using SizeType = fetch::math::SizeType;

template <class DataType>
void BM_TensorDataLoader_AddData(benchmark::State &state)
{
  using TensorType = fetch::math::Tensor<DataType>;

  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  auto n_datapoints = static_cast<SizeType>(state.range(0));
  auto input_size   = static_cast<SizeType>(state.range(1));
  auto output_size  = static_cast<SizeType>(state.range(2));
  auto n_inputs     = static_cast<SizeType>(state.range(3));

  // Prepare data and labels
  TensorType data({input_size, n_datapoints});
  TensorType labels({output_size, n_datapoints});
  data.FillUniformRandom();
  labels.FillUniformRandom();

  // make a dataloader
  auto dl = fetch::ml::dataloaders::TensorDataLoader<TensorType>();

  std::vector<TensorType> data_vector{};
  data_vector.reserve(n_inputs);
  for (SizeType i = 0; i < n_inputs; i++)
  {
    data_vector.emplace_back(data);
  }

  state.counters["charge"] = static_cast<double>(dl.ChargeAddData(data_vector, labels));

  for (auto _ : state)
  {
    // Initialise Optimiser
    dl.AddData(data_vector, labels);
  }
}

static void TensorDataLoaderAddDataArguments(benchmark::internal::Benchmark *b)
{
  std::vector<std::int64_t> data_sizes{1, 10, 100, 1000, 10000};
  std::vector<std::int64_t> input_sizes{1, 10, 100};
  std::vector<std::int64_t> output_sizes{1, 100, 10000};
  std::vector<std::int64_t> n_inputs{1, 10, 100};

  for (auto const &ds : data_sizes)
  {
    for (auto const &is : input_sizes)
    {
      for (auto const &os : output_sizes)
      {
        for (auto const &ins : n_inputs)
        {
          b->Args({ds, is, os, ins});
        }
      }
    }
  }
}

BENCHMARK_TEMPLATE(BM_TensorDataLoader_AddData, fetch::fixed_point::fp64_t)
    ->Apply(TensorDataLoaderAddDataArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_AddData, float)
    ->Apply(TensorDataLoaderAddDataArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_AddData, double)
    ->Apply(TensorDataLoaderAddDataArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_AddData, fetch::fixed_point::fp32_t)
    ->Apply(TensorDataLoaderAddDataArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_AddData, fetch::fixed_point::fp128_t)
    ->Apply(TensorDataLoaderAddDataArguments)
    ->Unit(::benchmark::kNanosecond);

template <class DataType>
void BM_TensorDataLoader_GetNext(benchmark::State &state)
{
  using TensorType = fetch::math::Tensor<DataType>;

  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  auto n_datapoints = static_cast<SizeType>(state.range(0));
  auto input_size   = static_cast<SizeType>(state.range(1));
  auto output_size  = static_cast<SizeType>(state.range(2));
  auto n_inputs     = static_cast<SizeType>(state.range(3));

  // Prepare data and labels
  TensorType data({input_size, n_datapoints});
  TensorType labels({output_size, n_datapoints});
  data.FillUniformRandom();
  labels.FillUniformRandom();

  std::vector<TensorType> data_vector{};
  data_vector.reserve(n_inputs);
  for (SizeType i = 0; i < n_inputs; i++)
  {
    data_vector.emplace_back(data);
  }

  // make a dataloader
  auto dl = fetch::ml::dataloaders::TensorDataLoader<TensorType>();
  dl.AddData({data_vector}, labels);

  state.counters["charge"] = static_cast<double>(dl.ChargeGetNext());

  for (auto _ : state)
  {
    // Initialise Optimiser
    dl.GetNext();
    if (dl.IsDone())
    {
      dl.Reset();
    }
  }
}

static void TensorDataLoaderGetNextArguments(benchmark::internal::Benchmark *b)
{
  std::vector<std::int64_t> data_sizes{1, 10, 100};
  std::vector<std::int64_t> input_sizes{1, 10, 100};
  std::vector<std::int64_t> output_sizes{1, 100, 10000};
  std::vector<std::int64_t> n_inputs{1, 10, 100};

  for (auto const &ds : data_sizes)
  {
    for (auto const &is : input_sizes)
    {
      for (auto const &os : output_sizes)
      {
        for (auto const &ins : n_inputs)
        {
          b->Args({ds, is, os, ins});
        }
      }
    }
  }
}

BENCHMARK_TEMPLATE(BM_TensorDataLoader_GetNext, float)
    ->Apply(TensorDataLoaderGetNextArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_GetNext, double)
    ->Apply(TensorDataLoaderGetNextArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_GetNext, fetch::fixed_point::fp32_t)
    ->Apply(TensorDataLoaderGetNextArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_GetNext, fetch::fixed_point::fp64_t)
    ->Apply(TensorDataLoaderGetNextArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TensorDataLoader_GetNext, fetch::fixed_point::fp128_t)
    ->Apply(TensorDataLoaderGetNextArguments)
    ->Unit(::benchmark::kNanosecond);

BENCHMARK_MAIN();
