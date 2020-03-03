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

#include "math/activation_functions/softmax.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"

#include "benchmark/benchmark.h"

#include <iostream>
#include <memory>
#include <vector>

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
void BM_MeanSquareErrorLossForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  // Get args form state
  BM_Tensor_config config{state};

  fetch::math::Tensor<T> input_1(config.shape);
  fetch::math::Tensor<T> input_2(config.shape);
  fetch::math::Tensor<T> output(config.shape);

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();
  output.FillUniformRandom();

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::MeanSquareErrorLoss<fetch::math::Tensor<T>> msqe1;

  state.counters["charge_total"] =
      static_cast<double>(msqe1.ChargeForward({config.shape, config.shape}).first);
  state.counters["charge_iterate"] = static_cast<double>(TensorType::ChargeIterate(config.shape));

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  for (auto _ : state)
  {
    msqe1.Forward(inputs, output);
  }
}

static void MeanSquareErrorLossArguments(benchmark::internal::Benchmark *b)
{
  using SizeType                   = fetch::math::SizeType;
  SizeType const N_ELEMENTS        = 3;
  std::int64_t   MAX_SIZE          = 2097152;
  std::int64_t   MAX_COMBINED_SIZE = 1024;

  std::vector<std::int64_t> dim_size;
  std::int64_t              i{1};
  while (i <= MAX_SIZE)
  {
    dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, j, 1, 1});
  }
  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, 1, j, 1});
  }

  std::vector<std::int64_t> combined_dim_size;
  i = 1;
  while (i <= MAX_COMBINED_SIZE)
  {
    combined_dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : combined_dim_size)
  {
    b->Args({3, j, j, 1});
  }
}

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::fp64_t)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, float)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, double)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::fp32_t)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::fp128_t)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);

template <class T>
void BM_MeanSquareErrorLossBackward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  // Get args form state
  BM_Tensor_config config{state};

  auto error_signal = TensorType(config.shape);

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(config.shape));
  inputs.emplace_back(std::make_shared<TensorType>(config.shape));
  fetch::ml::ops::MeanSquareErrorLoss<TensorType> msqe;

  msqe.SetBatchInputShapes({config.shape, config.shape});
  msqe.SetBatchOutputShape(config.shape);

  state.counters["charge_total"] =
      static_cast<double>(msqe.ChargeBackward({config.shape, config.shape}).first);
  state.counters["charge_iterate"] = static_cast<double>(TensorType::ChargeIterate(config.shape));

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  for (auto _ : state)
  {
    msqe.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::fp64_t)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, float)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, double)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::fp32_t)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::fp128_t)
    ->Apply(MeanSquareErrorLossArguments)
    ->Unit(::benchmark::kNanosecond);

template <class T>
void BM_CrossEntropyLossForward(benchmark::State &state)
{
  using TensorType    = typename fetch::math::Tensor<T>;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;

  // Get args form state
  BM_Tensor_config config{state};

  fetch::math::Tensor<T> input_1(config.shape);
  fetch::math::Tensor<T> input_2(config.shape);
  fetch::math::Tensor<T> output(config.shape);

  // Fill tensors with random values
  input_1.FillUniformRandomIntegers(0, 1);
  input_2.FillUniformRandomIntegers(0, 1);
  output.FillUniformRandomIntegers(0, 1);

  VecTensorType inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input_1));
  inputs.emplace_back(std::make_shared<TensorType>(input_2));
  fetch::ml::ops::CrossEntropyLoss<fetch::math::Tensor<T>> cel1;

  state.counters["charge_total"] =
      static_cast<double>(cel1.ChargeForward({config.shape, config.shape}).first);
  state.counters["charge_iterate"] = static_cast<double>(TensorType::ChargeIterate(config.shape));

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  for (auto _ : state)
  {
    cel1.Forward(inputs, output);
  }
}

static void CrossEntropyLossArguments(benchmark::internal::Benchmark *b)
{
  using SizeType                   = fetch::math::SizeType;
  SizeType const N_ELEMENTS        = 2;
  std::int64_t   MAX_SIZE          = 2097152;
  std::int64_t   MAX_COMBINED_SIZE = 1024;

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

  std::vector<std::int64_t> combined_dim_size;
  i = 1;
  while (i <= MAX_COMBINED_SIZE)
  {
    combined_dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : combined_dim_size)
  {
    b->Args({N_ELEMENTS, j, j});
  }
}

BENCHMARK_TEMPLATE(BM_CrossEntropyLossForward, fetch::fixed_point::fp64_t)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyLossForward, float)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyLossForward, double)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyLossForward, fetch::fixed_point::fp32_t)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyLossForward, fetch::fixed_point::fp128_t)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);

template <class T>
void BM_CrossEntropyBackward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  // Get args form state
  BM_Tensor_config config{state};

  auto error_signal = TensorType(config.shape);

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(config.shape));
  inputs.emplace_back(std::make_shared<TensorType>(config.shape));
  fetch::ml::ops::CrossEntropyLoss<TensorType> ce;

  state.counters["charge_total"] =
      static_cast<double>(ce.ChargeBackward({config.shape, config.shape}).first);
  state.counters["charge_iterate"] = static_cast<double>(TensorType::ChargeIterate(config.shape));

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  for (auto _ : state)
  {
    ce.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::fp64_t)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, float)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, double)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::fp32_t)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::fp128_t)
    ->Apply(CrossEntropyLossArguments)
    ->Unit(::benchmark::kNanosecond);

template <class T, int I, int B>
void BM_SoftmaxCrossEntropyLossForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  // using DataType      = typename TensorType::Type;

  auto test_results = TensorType({I, B});
  auto ground_truth = TensorType({I, B});
  auto output       = TensorType({I, B});

  test_results.FillUniformRandom();
  ground_truth.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(test_results));
  inputs.emplace_back(std::make_shared<TensorType>(fetch::math::Softmax(ground_truth)));
  fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType> sce;

  for (auto _ : state)
  {
    sce.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, float, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, float, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, float, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, float, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, double, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, double, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, double, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, double, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, double, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<16, 16>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<16, 16>, 10,
                   10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<16, 16>, 100,
                   100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<16, 16>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<16, 16>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<32, 32>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<32, 32>, 10,
                   10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<32, 32>, 100,
                   100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<32, 32>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<32, 32>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<64, 64>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<64, 64>, 10,
                   10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<64, 64>, 100,
                   100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<64, 64>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossForward, fetch::fixed_point::FixedPoint<64, 64>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

template <class T, int I, int B>
void BM_SoftmaxCrossEntropyLossBackward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  auto test_results = TensorType({I, B});
  auto ground_truth = TensorType({I, B});
  auto error_signal = TensorType({I, B});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(test_results));
  inputs.emplace_back(std::make_shared<TensorType>(fetch::math::Softmax(ground_truth)));
  fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType> sce;

  for (auto _ : state)
  {
    sce.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, float, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, float, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, float, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, float, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, double, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, double, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, double, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, double, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, double, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 10,
                   10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 100,
                   100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 10,
                   10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 100,
                   100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 10,
                   10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 100,
                   100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_SoftmaxCrossEntropyLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
