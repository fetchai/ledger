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

template <class T, int I, int B>
void BM_CrossEntropyForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  auto test_results = TensorType({I, B});
  auto ground_truth = TensorType({I, B});
  auto output       = TensorType({I, B});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(test_results));
  inputs.emplace_back(std::make_shared<TensorType>(ground_truth));
  fetch::ml::ops::CrossEntropyLoss<TensorType> ce;

  for (auto _ : state)
  {
    ce.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_CrossEntropyForward, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, float, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, float, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, float, 1000, 1000)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, float, 2000, 2000)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyForward, double, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, double, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, double, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, double, 1000, 1000)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, double, 2000, 2000)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<16, 16>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<16, 16>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<16, 16>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<16, 16>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<16, 16>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<32, 32>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<32, 32>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<32, 32>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<32, 32>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<32, 32>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<64, 64>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<64, 64>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<64, 64>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<64, 64>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyForward, fetch::fixed_point::FixedPoint<64, 64>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

template <class T, int I, int B>
void BM_CrossEntropyBackward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  auto test_results = TensorType({I, B});
  auto ground_truth = TensorType({I, B});
  auto error_signal = TensorType({I, B});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(test_results));
  inputs.emplace_back(std::make_shared<TensorType>(ground_truth));
  fetch::ml::ops::CrossEntropyLoss<TensorType> ce;

  for (auto _ : state)
  {
    ce.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, float, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, float, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, float, 1000, 1000)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, float, 2000, 2000)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, double, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, double, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, double, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, double, 1000, 1000)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, double, 2000, 2000)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<16, 16>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<16, 16>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<16, 16>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<16, 16>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<16, 16>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<32, 32>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<32, 32>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<32, 32>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<32, 32>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<32, 32>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<64, 64>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<64, 64>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<64, 64>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<64, 64>, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_CrossEntropyBackward, fetch::fixed_point::FixedPoint<64, 64>, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

template <class T, int I, int B>
void BM_MeanSquareErrorLossForward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  auto test_results = TensorType({I, B});
  auto ground_truth = TensorType({I, B});
  auto output       = TensorType({I, B});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(test_results));
  inputs.emplace_back(std::make_shared<TensorType>(ground_truth));
  fetch::ml::ops::MeanSquareErrorLoss<TensorType> mse;

  for (auto _ : state)
  {
    mse.Forward(inputs, output);
  }
}

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, float, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, float, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, float, 1000, 1000)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, float, 2000, 2000)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, double, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, double, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, double, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, double, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, double, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<16, 16>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<16, 16>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<16, 16>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<16, 16>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<16, 16>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<32, 32>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<32, 32>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<32, 32>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<32, 32>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<32, 32>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<64, 64>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<64, 64>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<64, 64>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<64, 64>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossForward, fetch::fixed_point::FixedPoint<64, 64>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

template <class T, int I, int B>
void BM_MeanSquareErrorLossBackward(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  auto test_results = TensorType({I, B});
  auto ground_truth = TensorType({I, B});
  auto error_signal = TensorType({I, B});

  std::vector<std::shared_ptr<fetch::math::Tensor<T> const>> inputs;
  inputs.emplace_back(std::make_shared<TensorType>(test_results));
  inputs.emplace_back(std::make_shared<TensorType>(ground_truth));
  fetch::ml::ops::MeanSquareErrorLoss<TensorType> mse;

  for (auto _ : state)
  {
    mse.Backward(inputs, error_signal);
  }
}

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, float, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, float, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, float, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, float, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, double, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, double, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, double, 100, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, double, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, double, 2000, 2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<16, 16>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<32, 32>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 2, 2)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 10, 10)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 1000,
                   1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MeanSquareErrorLossBackward, fetch::fixed_point::FixedPoint<64, 64>, 2000,
                   2000)
    ->Unit(benchmark::kMillisecond);

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
