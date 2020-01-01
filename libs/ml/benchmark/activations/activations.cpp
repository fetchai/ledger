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
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/elu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"

#include "benchmark/benchmark.h"

#include <functional>
#include <memory>
#include <vector>

template <class T, int N>
void BM_DropoutForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  auto input       = TensorType({1, N});
  auto output      = TensorType({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Dropout<fetch::math::Tensor<T>> dm(0.5);

  for (auto _ : state)
  {
    dm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_DropoutForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DropoutForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_DropoutBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto output       = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Dropout<fetch::math::Tensor<T>> dm(0.5);
  dm.Forward(inputs, output);

  for (auto _ : state)
  {
    dm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_DropoutBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_DropoutBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_DropoutBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_EluForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  TensorType input({1, N});
  TensorType output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));

  auto                                        a = T(0.2);
  fetch::ml::ops::Elu<fetch::math::Tensor<T>> em(a);

  for (auto _ : state)
  {
    em.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_EluForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_EluForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_EluBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  auto                                        a = T(0.2);
  fetch::ml::ops::Elu<fetch::math::Tensor<T>> em(a);

  for (auto _ : state)
  {
    em.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_EluBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_EluBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_EluBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LeakyReluForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  fetch::math::Tensor<T> input({1, N});
  fetch::math::Tensor<T> output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::LeakyRelu<fetch::math::Tensor<T>> lrm;

  for (auto _ : state)
  {
    lrm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_LeakyReluForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LeakyReluBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  fetch::ml::ops::LeakyRelu<fetch::math::Tensor<T>> lrm;

  for (auto _ : state)
  {
    lrm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_LeakyReluBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyReluBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LogSigmoidForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  TensorType input({1, N});
  TensorType output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::LogSigmoid<fetch::math::Tensor<T>> lsm;

  for (auto _ : state)
  {
    lsm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_LogSigmoidForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LogSigmoidBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  fetch::ml::ops::LogSigmoid<fetch::math::Tensor<T>> lsm;

  for (auto _ : state)
  {
    lsm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_LogSigmoidBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSigmoidBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LogSoftmaxForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  TensorType input({1, N});
  TensorType output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::LogSoftmax<fetch::math::Tensor<T>> lsm;

  for (auto _ : state)
  {
    lsm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_LogSoftmaxForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_LogSoftmaxBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  fetch::ml::ops::LogSoftmax<fetch::math::Tensor<T>> lsm;

  for (auto _ : state)
  {
    lsm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_LogSoftmaxBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LogSoftmaxBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_RandomisedReluForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  TensorType input({1, N});
  TensorType output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));

  auto                                                   lb = T(0.2);
  auto                                                   ub = T(0.8);
  fetch::ml::ops::RandomisedRelu<fetch::math::Tensor<T>> rrm(lb, ub);

  for (auto _ : state)
  {
    rrm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_RandomisedReluForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_RandomisedReluBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  auto                                                   lb = T(0.2);
  auto                                                   ub = T(0.8);
  fetch::ml::ops::RandomisedRelu<fetch::math::Tensor<T>> rrm(lb, ub);

  for (auto _ : state)
  {
    rrm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_RandomisedReluBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_RandomisedReluBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_ReluForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  TensorType input({1, N});
  TensorType output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Relu<fetch::math::Tensor<T>> rm;

  for (auto _ : state)
  {
    rm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_ReluForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReluForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_ReluBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  fetch::ml::ops::Relu<fetch::math::Tensor<T>> rm;

  for (auto _ : state)
  {
    rm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_ReluBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_ReluBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_ReluBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SigmoidForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  TensorType input({1, N});
  TensorType output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Sigmoid<fetch::math::Tensor<T>> sm;

  for (auto _ : state)
  {
    sm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SigmoidForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SigmoidBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  fetch::ml::ops::Sigmoid<fetch::math::Tensor<T>> sm;

  for (auto _ : state)
  {
    sm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_SigmoidBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SigmoidBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SigmoidBackward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SoftmaxForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  TensorType input({1, N});
  TensorType output({1, N});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(input));
  fetch::ml::ops::Softmax<fetch::math::Tensor<T>> sm;

  for (auto _ : state)
  {
    sm.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_SoftmaxForward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxForward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxForward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxForward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxForward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxForward, double, 4096)->Unit(benchmark::kMicrosecond);

template <class T, int N>
void BM_SoftmaxBackward(benchmark::State &state)
{
  using TensorType  = typename fetch::math::Tensor<T>;
  auto input        = TensorType({1, N});
  auto error_signal = TensorType({1, N});

  // Fill tensors with random values
  input.FillUniformRandom();
  error_signal.FillUniformRandom();

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;

  inputs.emplace_back(std::make_shared<TensorType>(input));

  fetch::ml::ops::Softmax<fetch::math::Tensor<T>> sm;

  for (auto _ : state)
  {
    sm.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_SoftmaxBackward, double, 2)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxBackward, double, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxBackward, double, 512)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxBackward, double, 1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxBackward, double, 2048)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_SoftmaxBackward, double, 4096)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
