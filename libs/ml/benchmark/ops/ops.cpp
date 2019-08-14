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
#include "ml/ops/add.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/ops.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/subtract.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

#include <vector>

template <typename T, int F, int N, int B>
void BM_MatrixMultiply_Forward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({F, N, B});
  fetch::math::Tensor<T> input_2({F, N, B});
  fetch::math::Tensor<T> output({F, N, B});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
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

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, double, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, double, 16, 16, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, double, 16, 16, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, double, 256, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, double, 256, 256, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Forward, double, 256, 256, 100)->Unit(benchmark::kMillisecond);

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
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({F, N, B});
  fetch::math::Tensor<T> input_2({F, N, B});
  fetch::math::Tensor<T> err_sig({F, N, B});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  err_sig.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
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

BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, double, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, double, 16, 16, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, double, 16, 16, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, double, 256, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, double, 256, 256, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MatrixMultiply_Backward, double, 256, 256, 100)
    ->Unit(benchmark::kMillisecond);

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
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> output({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Sqrt<fetch::math::Tensor<T>> sqrt1;

  for (auto _ : state)
  {
    sqrt1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SqrtForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqrtForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SqrtBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Sqrt<fetch::math::Tensor<T>> sqrt1;

  for (auto _ : state)
  {
    sqrt1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_SqrtBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqrtBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqrtBackward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LogForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> output({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Log<fetch::math::Tensor<T>> log1;

  for (auto _ : state)
  {
    log1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_LogForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LogForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LogBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Log<fetch::math::Tensor<T>> log1;

  for (auto _ : state)
  {
    log1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_LogBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LogBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_ExpForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> output({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Exp<fetch::math::Tensor<T>> exp1;

  for (auto _ : state)
  {
    exp1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_ExpForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ExpForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_ExpBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Exp<fetch::math::Tensor<T>> exp1;

  for (auto _ : state)
  {
    exp1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_ExpBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ExpBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ExpBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_DivideForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> output({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Divide<fetch::math::Tensor<T>> div1;

  for (auto _ : state)
  {
    div1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_DivideForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_DivideForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideForward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_DivideBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Divide<fetch::math::Tensor<T>> div1;

  for (auto _ : state)
  {
    div1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_DivideBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_DivideBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DivideBackward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_MultiplyForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> output({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Multiply<fetch::math::Tensor<T>> mul1;

  for (auto _ : state)
  {
    mul1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MultiplyForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MultiplyForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyForward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_MultiplyBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Multiply<fetch::math::Tensor<T>> mul1;

  for (auto _ : state)
  {
    mul1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_MultiplyBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MultiplyBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp32_t, 2)
    ->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp64_t, 2)
    ->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MultiplyBackward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_AddForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> output({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Add<fetch::math::Tensor<T>> add1;

  for (auto _ : state)
  {
    add1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_AddForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AddForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_AddBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Add<fetch::math::Tensor<T>> add1;

  for (auto _ : state)
  {
    add1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_AddBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AddBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SubtractForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> output({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Subtract<fetch::math::Tensor<T>> subtract1;

  for (auto _ : state)
  {
    subtract1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SubtractForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SubtractForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractForward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SubtractBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({1, N});
  fetch::math::Tensor<T> input_2({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::Subtract<fetch::math::Tensor<T>> sub1;

  for (auto _ : state)
  {
    sub1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_SubtractBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SubtractBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp32_t, 2)
    ->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp64_t, 2)
    ->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SubtractBackward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
