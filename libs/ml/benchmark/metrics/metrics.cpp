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
#include "ml/ops/metrics/categorical_accuracy.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

template <class T, int I, int B>
void BM_CategoricalAccuracy(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto test_results = TensorType({I, B});
  auto ground_truth = TensorType({I, B});
  auto output       = TensorType({I, B});

  // Fill tensors with random values
  test_results.FillUniformRandom();
  ground_truth.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(test_results));
  inputs.emplace_back(std::make_shared<TensorType>(ground_truth));

  fetch::ml::ops::CategoricalAccuracy<TensorType> ca;

  for (auto _ : state)
  {
    ca.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, float, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, float, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, float, 1000, 1000)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, float, 2000, 2000)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, double, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, double, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, double, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, double, 1000, 1000)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, double, 2000, 2000)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<16, 16>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<16, 16>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<16, 16>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<16, 16>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<16, 16>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<32, 32>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<32, 32>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<32, 32>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<32, 32>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<32, 32>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<64, 64>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<64, 64>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<64, 64>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<64, 64>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CategoricalAccuracy, fetch::fixed_point::FixedPoint<64, 64>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
