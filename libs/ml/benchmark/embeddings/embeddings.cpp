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

#include "logging/logging.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/lazy_adam_optimiser.hpp"

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

//////////////////////////
/// reusable functions ///
//////////////////////////

template <typename TypeParam>
std::shared_ptr<fetch::ml::Graph<TypeParam>> PrepareTestGraph(
    typename TypeParam::SizeType embedding_dimensions, typename TypeParam::SizeType n_datapoints,
    std::string &input_name, std::string &label_name, std::string &error_name)
{
  std::shared_ptr<fetch::ml::Graph<TypeParam>> g(std::make_shared<fetch::ml::Graph<TypeParam>>());

  input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string output_name = g->template AddNode<fetch::ml::ops::Embeddings<TypeParam>>(
      "Embeddings", {input_name}, embedding_dimensions, n_datapoints);

  label_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});
  error_name = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "Error", {output_name, label_name});

  return g;
}

template <typename T, fetch::math::SizeType B, fetch::math::SizeType S, fetch::math::SizeType D,
          fetch::math::SizeType E>
void BM_Setup_And_Train_Embeddings_Adam(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  SizeType batch_size           = B;
  SizeType embedding_dimensions = S;
  SizeType n_datapoints         = D;
  SizeType n_epochs             = E;

  auto learning_rate = DataType{0.1f};

  // Prepare data and labels
  TensorType data({1, batch_size});
  TensorType gt({embedding_dimensions, 1, batch_size});
  data.FillUniformRandomIntegers(0, n_datapoints);
  gt.FillUniformRandom();

  for (auto _ : state)
  {
    // make a graph
    std::string input_name;
    std::string label_name;
    std::string error_name;
    auto        g = PrepareTestGraph<TensorType>(embedding_dimensions, n_datapoints, input_name,
                                          label_name, error_name);

    // Initialise Optimiser
    fetch::ml::optimisers::AdamOptimiser<TensorType> optimiser(g, {input_name}, label_name,
                                                               error_name, learning_rate);

    // Do optimisation
    for (SizeType i = 0; i < n_epochs; ++i)
    {
      data.FillUniformRandomIntegers(0, n_datapoints);
      optimiser.Run({data}, gt);
    }
  }
}

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 1, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 10, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 100, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 1000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 10000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 20000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 30000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 40000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 50000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 60000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 70000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 80000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 90000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_Adam, float, 100000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);

template <typename T, fetch::math::SizeType B, fetch::math::SizeType S, fetch::math::SizeType D,
          fetch::math::SizeType E>
void BM_Setup_And_Train_Embeddings_LazyAdam(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  SizeType batch_size           = B;
  SizeType embedding_dimensions = S;
  SizeType n_datapoints         = D;
  SizeType n_epochs             = E;

  auto learning_rate = DataType{0.1f};

  // Prepare data and labels
  TensorType data({1, batch_size});
  TensorType gt({embedding_dimensions, 1, batch_size});
  data.FillUniformRandomIntegers(0, n_datapoints);
  gt.FillUniformRandom();

  for (auto _ : state)
  {
    // make a graph
    std::string input_name;
    std::string label_name;
    std::string error_name;
    auto        g = PrepareTestGraph<TensorType>(embedding_dimensions, n_datapoints, input_name,
                                          label_name, error_name);

    // Initialise Optimiser
    fetch::ml::optimisers::LazyAdamOptimiser<TensorType> optimiser(g, {input_name}, label_name,
                                                                   error_name, learning_rate);

    // Do optimisation
    for (SizeType i = 0; i < n_epochs; ++i)
    {
      data.FillUniformRandomIntegers(0, n_datapoints);
      optimiser.Run({data}, gt);
    }
  }
}

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 1, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 10, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 100, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 1000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 10000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 20000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 30000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 40000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 50000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 60000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 70000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 80000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 90000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings_LazyAdam, float, 100000, 500, 10000, 10)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
