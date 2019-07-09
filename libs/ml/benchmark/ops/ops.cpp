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

#include "math/tensor.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/sqrt.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

#include <vector>

template <class T, int F, int N, int B>
void BM_MatrixMultiply_Forward(benchmark::State &state)
{
  fetch::math::Tensor<T> input_1({F, N, B});
  fetch::math::Tensor<T> input_2({F, N, B});
  fetch::math::Tensor<T> output({F, N, B});

  std::vector<std::reference_wrapper<fetch::math::Tensor<T> const>> inputs;
  inputs.push_back(input_1);
  inputs.push_back(input_2);
  fetch::ml::ops::MatrixMultiply<fetch::math::Tensor<T>> matmul;

  for (auto _ : state)
  {
    matmul.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, float, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, float, 16, 16, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, float, 16, 16, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, float, 256, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, float, 256, 256, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, float, 256, 256, 100)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp32_t, 16, 16, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp32_t, 16, 16, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp32_t, 16, 16, 100)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp32_t, 256, 256, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp32_t, 256, 256, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp32_t, 256, 256, 100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp64_t, 16, 16, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp64_t, 16, 16, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp64_t, 16, 16, 100)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp64_t, 256, 256, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp64_t, 256, 256, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, fetch::fixed_point::fp64_t, 256, 256, 100)
    ->Unit(benchmark::kMillisecond);

// TODO () : also benchmark for fp128_t

template <class T, int F, int N, int B>
void BM_MatrixMultiply_Backward(benchmark::State &state)
{
  fetch::math::Tensor<T> input_1({F, N, B});
  fetch::math::Tensor<T> input_2({F, N, B});
  fetch::math::Tensor<T> err_sig({F, N, B});

  std::vector<std::reference_wrapper<fetch::math::Tensor<T> const>> inputs;
  inputs.push_back(input_1);
  inputs.push_back(input_2);
  fetch::ml::ops::MatrixMultiply<fetch::math::Tensor<T>> matmul;

  for (auto _ : state)
  {
    matmul.Backward(inputs, err_sig);
  }
}

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, float, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, float, 16, 16, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, float, 16, 16, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, float, 256, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, float, 256, 256, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, float, 256, 256, 100)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp32_t, 16, 16, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp32_t, 16, 16, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp32_t, 16, 16, 100)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp32_t, 256, 256, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp32_t, 256, 256, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp32_t, 256, 256, 100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp64_t, 16, 16, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp64_t, 16, 16, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp64_t, 16, 16, 100)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp64_t, 256, 256, 1)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp64_t, 256, 256, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, fetch::fixed_point::fp64_t, 256, 256, 100)
    ->Unit(benchmark::kMillisecond);

template <class T, int N>
void BM_SqrtForward(benchmark::State &state)
{
  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> output({1, N});

  std::vector<std::reference_wrapper<fetch::math::Tensor<T> const>> inputs;
  inputs.push_back(input);
  fetch::ml::ops::Sqrt<fetch::math::Tensor<T>> sqrt1;

  for (auto _ : state)
  {
    sqrt1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SqrtForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
