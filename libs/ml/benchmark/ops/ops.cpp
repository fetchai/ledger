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
#include "ml/ops/abs.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/avg_pool_1d.hpp"
#include "ml/ops/avg_pool_2d.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/mask_fill.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/maximum.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/one_hot.hpp"
#include "ml/ops/prelu_op.hpp"
#include "ml/ops/reduce_mean.hpp"
#include "ml/ops/reshape.hpp"
#include "ml/ops/slice.hpp"
#include "ml/ops/ops.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/squeeze.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/ops/switch.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/top_k.hpp"
#include "ml/ops/transpose.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

#include <vector>

template <class T, int N>
void BM_AbsForward(benchmark::State &state)
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
  fetch::ml::ops::Abs<fetch::math::Tensor<T>> abs;

  for (auto _ : state)
  {
	  abs.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_AbsForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AbsForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_AbsBackward(benchmark::State &state)
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
  fetch::ml::ops::Abs<fetch::math::Tensor<T>> abs;

  for (auto _ : state)
  {
	  abs.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_AbsBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AbsBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AbsBackward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N, int K, int S>
void BM_AvgPool1DForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::AvgPool1D<TensorType> avg_pool_1d(K, S);
  fetch::math::Tensor<T> output(avg_pool_1d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  avg_pool_1d.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, float, 256, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, double, 256, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp32_t, 256, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DForward, fetch::fixed_point::fp64_t, 256, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N, int C, int K, int S>
void BM_AvgPool1DBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({C, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::AvgPool1D<TensorType> avg_pool_1d(K, S);
  fetch::math::Tensor<T> error_signal(avg_pool_1d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  avg_pool_1d.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, float, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, double, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp32_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool1DBackward, fetch::fixed_point::fp64_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N, int C, int K, int S>
void BM_AvgPool2DForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({C, N, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::AvgPool2D<TensorType> avg_pool_2d(K, S);
  fetch::math::Tensor<T> output(avg_pool_2d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  avg_pool_2d.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, float, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, double, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp32_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DForward, fetch::fixed_point::fp64_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N, int C, int K, int S>
void BM_AvgPool2DBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({C, N, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::AvgPool2D<TensorType> avg_pool_2d(K, S);
  fetch::math::Tensor<T> output(avg_pool_2d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  avg_pool_2d.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, float, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, double, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp32_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AvgPool2DBackward, fetch::fixed_point::fp64_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_ConcatenateForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({N, 1});
  fetch::math::Tensor<T> input_2({N, 1});
  fetch::math::Tensor<T> output;

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));

  fetch::ml::ops::Concatenate<TensorType> concat(0);


  for (auto _ : state)
  {
	  concat.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_ConcatenateForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ConcatenateForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_ConcatenateBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, 1});
  fetch::math::Tensor<T> error_signal({N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));

  fetch::ml::ops::Concatenate<TensorType> concat(0);

  for (auto _ : state)
  {
	  concat.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_ConcatenateBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ConcatenateBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ConcatenateBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);


template <class T, int N, int C, int H, int K, int O>
void BM_Conv1DForward(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType input_channels  = C;
  SizeType input_height    = H;
  SizeType batch_size      = N;
  SizeType output_channels = O;
  SizeType kernel_height   = K;

  fetch::math::Tensor<T> input({input_channels, input_height, batch_size});
  fetch::math::Tensor<T> kernel({output_channels, input_channels, kernel_height, batch_size});

  // Fill tensors with random values
  input.FillUniformRandom();
  kernel.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  inputs.emplace_back(std::make_shared<TensorType>(kernel));

  fetch::ml::ops::Convolution1D<TensorType> conv_1d;
  fetch::math::Tensor<T> output(conv_1d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  conv_1d.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, float, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, double, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp32_t, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DForward, fetch::fixed_point::fp64_t, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);

template <class T, int N, int C, int H, int K, int O>
void BM_Conv1DBackward(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType input_channels  = C;
  SizeType input_height    = H;
  SizeType batch_size      = N;
  SizeType output_channels = O;
  SizeType kernel_height   = K;

  fetch::math::Tensor<T> input({input_channels, input_height, batch_size});
  fetch::math::Tensor<T> kernel({output_channels, input_channels, kernel_height, batch_size});

  // Fill tensors with random values
  input.FillUniformRandom();
  kernel.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  inputs.emplace_back(std::make_shared<TensorType>(kernel));

  fetch::ml::ops::Convolution1D<TensorType> conv_1d;
  fetch::math::Tensor<T> error_signal(conv_1d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  conv_1d.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, float, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, double, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp32_t, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 2, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 4, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 8, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 2, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 4, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 8, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 16, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 2, 16, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 4, 16, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv1DBackward, fetch::fixed_point::fp64_t, 1, 8, 16, 1, 16)->Unit(benchmark::kMicrosecond);


template <class T, int N, int C, int H, int W,  int K, int V, int O>
void BM_Conv2DForward(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType input_channels  = C;
  SizeType input_height    = H;
  SizeType input_width     = W;
  SizeType batch_size      = N;
  SizeType output_channels = O;
  SizeType kernel_height   = K;
  SizeType kernel_width    = V;

  fetch::math::Tensor<T> input({input_channels, input_height, input_width, batch_size});
  fetch::math::Tensor<T> kernel({output_channels, input_channels, kernel_height, kernel_width, batch_size});

  // Fill tensors with random values
  input.FillUniformRandom();
  kernel.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  inputs.emplace_back(std::make_shared<TensorType>(kernel));

  fetch::ml::ops::Convolution2D<TensorType> conv_2d;
  fetch::math::Tensor<T> output(conv_2d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  conv_2d.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, float, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, double, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DForward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);


template <class T, int N, int C, int H, int W,  int K, int V, int O>
void BM_Conv2DBackward(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType input_channels  = C;
  SizeType input_height    = H;
  SizeType input_width     = W;
  SizeType batch_size      = N;
  SizeType output_channels = O;
  SizeType kernel_height   = K;
  SizeType kernel_width    = V;

  fetch::math::Tensor<T> input({input_channels, input_height, input_width, batch_size});
  fetch::math::Tensor<T> kernel({output_channels, input_channels, kernel_height, kernel_width, batch_size});

  // Fill tensors with random values
  input.FillUniformRandom();
  kernel.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  inputs.emplace_back(std::make_shared<TensorType>(kernel));

  fetch::ml::ops::Convolution2D<TensorType> conv_2d;
  fetch::math::Tensor<T> output(conv_2d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  conv_2d.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, float, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, double, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp32_t, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 2, 2, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 4, 4, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 8, 8, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 2, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 4, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 8, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 16, 16, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 2, 2, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 4, 4, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 8, 8, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 16, 16, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 8)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Conv2DBackward, fetch::fixed_point::fp64_t, 1, 1, 16, 16, 1, 1, 16)->Unit(benchmark::kMicrosecond);

template <typename T, int N, int D, int P>
void BM_EmbeddingsForward(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType batch_size    = N;
  SizeType dimensions    = D;
  SizeType n_datapoints  = P;

  fetch::math::Tensor<T> input({1, batch_size});
  fetch::math::Tensor<T> output({dimensions, 1, batch_size});

  // Fill tensors with random values
  input.FillUniformRandomIntegers(0, static_cast<int64_t>(n_datapoints));
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Embeddings<fetch::math::Tensor<T>> emb(dimensions, n_datapoints);

  for (auto _ : state)
  {
	  emb.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, float, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, double, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp32_t, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsForward, fetch::fixed_point::fp64_t, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);

template <typename T, int N, int D, int P>
void BM_EmbeddingsBackward(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType batch_size    = N;
  SizeType dimensions    = D;
  SizeType n_datapoints  = P;

  fetch::math::Tensor<T> input({1, batch_size});
  fetch::math::Tensor<T> error_signal({dimensions, 1, batch_size});

  // Fill tensors with random values
  input.FillUniformRandomIntegers(0, static_cast<int64_t>(n_datapoints));
  error_signal.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Embeddings<fetch::math::Tensor<T>> emb(dimensions, n_datapoints);

  for (auto _ : state)
  {
	  emb.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, float, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);


BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, double, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp32_t, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 2, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 2, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 2, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 2, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 2, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 4, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 4, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 4, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 4, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 4, 1024, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 16, 16, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 16, 64, 64)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 16, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EmbeddingsBackward, fetch::fixed_point::fp64_t, 16, 1024, 1024)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_FlattenForward(benchmark::State &state)
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
  fetch::ml::ops::Flatten<fetch::math::Tensor<T>> flatten;

  for (auto _ : state)
  {
	flatten.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_FlattenForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_FlattenForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_FlattenBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> output({1, N});
  fetch::math::Tensor<T> error_signal({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Flatten<fetch::math::Tensor<T>> flatten;
  flatten.Forward(inputs, output);

  for (auto _ : state)
  {
	flatten.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_FlattenBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_FlattenBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_FlattenBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LayerNormForward(benchmark::State &state)
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
  fetch::ml::ops::LayerNorm<fetch::math::Tensor<T>> lnorm;

  for (auto _ : state)
  {
	  lnorm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_LayerNormForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LayerNormForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LayerNormBackward(benchmark::State &state)
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
  fetch::ml::ops::LayerNorm<fetch::math::Tensor<T>> lnorm;

  for (auto _ : state)
  {
	  lnorm.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_LayerNormBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LayerNormBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LayerNormBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_MaskFillForward(benchmark::State &state)
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

  T n(1.0);
  fetch::ml::ops::MaskFill<fetch::math::Tensor<T>> mfill(n);

  for (auto _ : state)
  {
	  mfill.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MaskFillForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaskFillForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_MaskFillBackward(benchmark::State &state)
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

  T n(1.0);
  fetch::ml::ops::MaskFill<fetch::math::Tensor<T>> mfill(n);

  for (auto _ : state)
  {
	  mfill.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MaskFillBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaskFillBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaskFillBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

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

template <class T, int N, int K, int S>
void BM_MaxPool1DForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::MaxPool1D<TensorType> max_pool_1d(K, S);
  fetch::math::Tensor<T> output(max_pool_1d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  max_pool_1d.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, float, 256, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, double, 256, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp32_t, 256, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 4, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 4, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 16, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 16, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 16, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 256, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 256, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DForward, fetch::fixed_point::fp64_t, 256, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N, int C, int K, int S>
void BM_MaxPool1DBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({C, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::MaxPool1D<TensorType> max_pool_1d(K, S);
  fetch::math::Tensor<T> error_signal(max_pool_1d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  max_pool_1d.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, float, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, double, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp32_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool1DBackward, fetch::fixed_point::fp64_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N, int C, int K, int S>
void BM_MaxPool2DForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({C, N, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::MaxPool2D<TensorType> max_pool_2d(K, S);
  fetch::math::Tensor<T> output(max_pool_2d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  max_pool_2d.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, float, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, double, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp32_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DForward, fetch::fixed_point::fp64_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N, int C, int K, int S>
void BM_MaxPool2DBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, C, C, N});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::MaxPool2D<TensorType> max_pool_2d(K, S);
  fetch::math::Tensor<T> error_signal(max_pool_2d.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  max_pool_2d.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, float, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, double, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp32_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 4, 1, 1, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 4, 2, 2, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 4, 4, 4, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 16, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 16, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 16, 4, 4, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 256, 1, 1, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 256, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaxPool2DBackward, fetch::fixed_point::fp64_t, 256, 4, 4, 4)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_MaximumForward(benchmark::State &state)
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
  fetch::ml::ops::Maximum<fetch::math::Tensor<T>> max;

  for (auto _ : state)
  {
	  max.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MaximumForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaximumForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_MaximumBackward(benchmark::State &state)
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
  fetch::ml::ops::Maximum<fetch::math::Tensor<T>> max;

  for (auto _ : state)
  {
	  max.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MaximumBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaximumBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MaximumBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);


template <class T, int N, int D>
void BM_OneHotForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType depth = D;

  fetch::math::Tensor<T> input({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::OneHot<fetch::math::Tensor<T>> one_hot(depth);
  fetch::math::Tensor<T> output(one_hot.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  one_hot.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_OneHotForward, float, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, float, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, float, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, float, 2048, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, double, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, double, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, double, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, double, 2048, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp32_t, 2048, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotForward, fetch::fixed_point::fp64_t, 2048, 16)->Unit(benchmark::kMicrosecond);

template <class T, int N, int D>
void BM_OneHotBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  SizeType depth = D;

  fetch::math::Tensor<T> input({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::OneHot<fetch::math::Tensor<T>> one_hot(depth);
  fetch::math::Tensor<T> output(one_hot.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  one_hot.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, float, 2048, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, double, 2048, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp32_t, 2048, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 2, 1)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 256, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 512, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 1024, 1)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 2048, 1)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 2, 4)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 256, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 512, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 1024, 4)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 2048, 4)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 2, 16)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 256, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 512, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 1024, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_OneHotBackward, fetch::fixed_point::fp64_t, 2048, 16)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_PreluForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({N, 1});
  fetch::math::Tensor<T> input_2({N, 1});
  fetch::math::Tensor<T> output({N, 1});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::PReluOp<fetch::math::Tensor<T>> prelu;

  for (auto _ : state)
  {
	  prelu.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_PreluForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_PreluForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_PreluBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({N, 1});
  fetch::math::Tensor<T> input_2({N, 1});
  fetch::math::Tensor<T> output({N, 1});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::PReluOp<fetch::math::Tensor<T>> prelu;

  for (auto _ : state)
  {
	  prelu.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_PreluBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_PreluBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_PreluBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_ReduceMeanForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  SizeType axis = 1;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::ReduceMean<fetch::math::Tensor<T>> rmean(axis);
  fetch::math::Tensor<T> output(rmean.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  rmean.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_ReduceMeanForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ReduceMeanForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);


template <class T, int N>
void BM_ReduceMeanBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  SizeType axis = 1;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::ReduceMean<fetch::math::Tensor<T>> rmean(axis);
  fetch::math::Tensor<T> output(rmean.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  rmean.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReduceMeanBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N, int M>
void BM_ReshapeForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, M, 1});
  std::vector<SizeType> new_shape({M, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Reshape<fetch::math::Tensor<T>> reshape(new_shape);
  fetch::math::Tensor<T> output(reshape.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  reshape.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_ReshapeForward, float, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, float, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, float, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, float, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, float, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, float, 4096, 8192)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ReshapeForward, double, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, double, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, double, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, double, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, double, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, double, 4096, 8192)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp32_t, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp32_t, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp32_t, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp32_t, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp32_t, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp32_t, 4096, 8192)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp64_t, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp64_t, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp64_t, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp64_t, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp64_t, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeForward, fetch::fixed_point::fp64_t, 4096, 8192)->Unit(benchmark::kMillisecond);

template <class T, int N, int M>
void BM_ReshapeBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, M, 1});
  std::vector<SizeType> new_shape({M, N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Reshape<fetch::math::Tensor<T>> reshape(new_shape);
  fetch::math::Tensor<T> output(reshape.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  reshape.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_ReshapeBackward, float, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, float, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, float, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, float, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, float, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, float, 4096, 8192)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ReshapeBackward, double, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, double, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, double, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, double, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, double, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, double, 4096, 8192)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp32_t, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp32_t, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp32_t, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp32_t, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp32_t, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp32_t, 4096, 8192)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp64_t, 2, 256)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp64_t, 256, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp64_t, 512, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp64_t, 1024, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp64_t, 2048, 4096)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_ReshapeBackward, fetch::fixed_point::fp64_t, 4096, 8192)->Unit(benchmark::kMillisecond);

template <class T, int N>
void BM_SliceForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  SizeType axis = 1;
  SizeType index = N - 1;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Slice<fetch::math::Tensor<T>> slice(index, axis);
  fetch::math::Tensor<T> output(slice.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  slice.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SliceForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SliceForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SliceBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  SizeType axis = 1;
  SizeType index = N - 1;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Slice<fetch::math::Tensor<T>> slice(index, axis);
  fetch::math::Tensor<T> output(slice.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  slice.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SliceBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SliceBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SliceBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SwitchForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({N, 1});
  fetch::math::Tensor<T> input_2({N, 1});
  fetch::math::Tensor<T> input_3({N, 1});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  input_3.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  inputs.emplace_back(std::make_shared<TensorType>(input_3));
  fetch::ml::ops::Switch<fetch::math::Tensor<T>> sw;
  fetch::math::Tensor<T> output(sw.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  sw.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SwitchForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SwitchForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SwitchBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input_1({N, 1});
  fetch::math::Tensor<T> input_2({N, 1});
  fetch::math::Tensor<T> input_3({N, 1});

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  input_3.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  inputs.emplace_back(std::make_shared<TensorType>(input_3));
  fetch::ml::ops::Switch<fetch::math::Tensor<T>> sw;
  fetch::math::Tensor<T> output(sw.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  sw.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SwitchBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SwitchBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SwitchBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_TanHForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::TanH<fetch::math::Tensor<T>> tanh;
  fetch::math::Tensor<T> output(tanh.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  tanh.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_TanHForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TanHForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_TanHBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::TanH<fetch::math::Tensor<T>> tanh;
  fetch::math::Tensor<T> output(tanh.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  tanh.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_TanHBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TanHBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TanHBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_TopKForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::TopK<fetch::math::Tensor<T>> topk(N-1);
  fetch::math::Tensor<T> output(topk.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  topk.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_TopKForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TopKForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_TopKBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::TopK<fetch::math::Tensor<T>> topk(N-1);
  fetch::math::Tensor<T> output(topk.ComputeOutputShape(inputs));

  topk.Forward(inputs, output);

  for (auto _ : state)
  {
	  topk.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_TopKBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TopKBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TopKBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_TransposeForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Transpose<fetch::math::Tensor<T>> tr;
  fetch::math::Tensor<T> output(tr.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  tr.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_TransposeForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TransposeForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeForward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_TransposeBackward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({N, 1});

  // Fill tensors with random values
  input.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Transpose<fetch::math::Tensor<T>> tr;
  fetch::math::Tensor<T> output(tr.ComputeOutputShape(inputs));

  for (auto _ : state)
  {
	  tr.Backward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_TransposeBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TransposeBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp32_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp32_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp32_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp32_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp32_t, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp64_t, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp64_t, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp64_t, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp64_t, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_TransposeBackward, fetch::fixed_point::fp64_t, 4096)->Unit(benchmark::kMicrosecond);

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

template <class T, int N>
void BM_SqueezeForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> output({N});

  // Fill tensors with random values
  input.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Squeeze<fetch::math::Tensor<T>> sque1;

  for (auto _ : state)
  {
    sque1.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SqueezeForward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqueezeForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeForward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SqueezeBackward(benchmark::State &state)
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
  fetch::ml::ops::Squeeze<fetch::math::Tensor<T>> sque1;

  for (auto _ : state)
  {
    sque1.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_SqueezeBackward, float, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, float, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, float, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, float, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, float, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, float, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqueezeBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp32_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp32_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp32_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp32_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp32_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp32_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp64_t, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp64_t, 256)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp64_t, 512)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp64_t, 1024)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp64_t, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SqueezeBackward, fetch::fixed_point::fp64_t, 4096)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
