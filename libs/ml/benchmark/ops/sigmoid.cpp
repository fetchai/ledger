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

#include <vector>

#include "benchmark/benchmark.h"
#include "math/tensor.hpp"
#include "ml/ops/activations/sigmoid.hpp"

template <class T, int N>
void BM_SigmoidForward(benchmark::State &state)
{
  fetch::math::Tensor<T>                                            input({1, N});
  fetch::math::Tensor<T>                                            output({1, N});
  std::vector<std::reference_wrapper<fetch::math::Tensor<T> const>> inputs;
  inputs.push_back(input);
  fetch::ml::ops::Sigmoid<fetch::math::Tensor<T>> sigmoid_module;
  for (auto _ : state)
  {
    benchmark::DoNotOptimize(sigmoid_module.Forward(inputs, output));
  }
}

BENCHMARK_TEMPLATE(BM_SigmoidForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, float, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, float, 512)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, float, 1024)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, float, 2048)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, float, 4096)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
