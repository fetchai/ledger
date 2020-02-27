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
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

using SizeType = fetch::math::SizeType;

template <class TensorType>
std::shared_ptr<fetch::ml::Graph<TensorType>> make_graph(SizeType input_size, SizeType n_hidden,
                                                         SizeType hidden_size, SizeType output_size,
                                                         std::string &input_name,
                                                         std::string &label_name,
                                                         std::string &error_name)
{
  auto g = std::make_shared<fetch::ml::Graph<TensorType>>();

  // set up the neural net architecture
  input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});
  label_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});

  std::string h_1 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {input_name}, input_size, hidden_size);
  std::string a_1 = g->template AddNode<fetch::ml::ops::Relu<TensorType>>("", {h_1});

  for (SizeType i = 0; i < n_hidden; i++)
  {
    std::string h_2 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
        "", {a_1}, hidden_size, hidden_size);
  }
  std::string h_2 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC2", {a_1}, hidden_size, output_size);
  std::string output_name = g->template AddNode<fetch::ml::ops::Relu<TensorType>>("", {h_2});

  error_name = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
      "", {output_name, label_name});
  return g;
}

template <class Optimiser>
void BM_Optimiser_Construct(benchmark::State &state)
{
  using TensorType = typename Optimiser::TensorType;
  using DataType   = typename Optimiser::DataType;

  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  SizeType batch_size  = static_cast<SizeType>(state.range(0));
  SizeType input_size  = static_cast<SizeType>(state.range(1));
  SizeType hidden_size = static_cast<SizeType>(state.range(2));
  SizeType output_size = static_cast<SizeType>(state.range(3));
  SizeType n_hidden    = static_cast<SizeType>(state.range(4));

  auto learning_rate = fetch::math::Type<DataType>("0.001");

  // Prepare data and labels
  TensorType data({input_size, batch_size});
  TensorType gt({output_size, batch_size});
  data.FillUniformRandom();
  gt.FillUniformRandom();

  // make a graph
  std::string input_name, label_name, error_name;
  auto        g = make_graph<TensorType>(input_size, n_hidden, hidden_size, output_size, input_name,
                                  label_name, error_name);

  state.counters["charge"] = static_cast<double>(Optimiser::ChargeConstruct(g));

  for (auto _ : state)
  {
    // Initialise Optimiser
    Optimiser optimiser(g, {input_name}, label_name, error_name, learning_rate);
  }
}
static void OptimiserConstructArguments(benchmark::internal::Benchmark *b)
{
  std::vector<std::int64_t> batch_sizes{1, 10, 100};
  std::vector<std::int64_t> input_sizes{1, 10, 100};
  std::vector<std::int64_t> hidden_sizes{1, 10, 100};
  std::vector<std::int64_t> output_sizes{1, 10, 100};
  std::int64_t              n_hidden = 10;

  SizeType n_benchmarks = batch_sizes.size();
  for (SizeType i = 0; i < n_benchmarks; i++)
  {
    b->Args({batch_sizes[i], input_sizes[i], hidden_sizes[i], output_sizes[i], n_hidden});
  }
}

BENCHMARK_TEMPLATE(BM_Optimiser_Construct,
                   fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<float>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Optimiser_Construct,
                   fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<double>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Construct,
    fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<fetch::fixed_point::fp32_t>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Construct,
    fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<fetch::fixed_point::fp64_t>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Construct,
    fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<fetch::fixed_point::fp128_t>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Optimiser_Construct,
                   fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<float>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Optimiser_Construct,
                   fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<double>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Construct,
    fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<fetch::fixed_point::fp32_t>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Construct,
    fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<fetch::fixed_point::fp64_t>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Construct,
    fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<fetch::fixed_point::fp128_t>>)
    ->Apply(OptimiserConstructArguments)
    ->Unit(::benchmark::kMicrosecond);

template <class Optimiser>
void BM_Optimiser_Run(benchmark::State &state)
{
  using TensorType = typename Optimiser::TensorType;
  using DataType   = typename Optimiser::DataType;

  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  // Get args from state
  SizeType batch_size  = static_cast<SizeType>(state.range(0));
  SizeType input_size  = static_cast<SizeType>(state.range(1));
  SizeType hidden_size = static_cast<SizeType>(state.range(2));
  SizeType output_size = static_cast<SizeType>(state.range(3));
  SizeType n_epochs    = static_cast<SizeType>(state.range(4));
  SizeType n_hidden    = static_cast<SizeType>(state.range(5));

  auto learning_rate = fetch::math::Type<DataType>("0.001");

  // Prepare data and labels
  TensorType data({input_size, batch_size});
  TensorType gt({output_size, batch_size});
  data.FillUniformRandom();
  gt.FillUniformRandom();

  // make a graph
  std::string input_name, label_name, error_name;
  auto        g = make_graph<TensorType>(input_size, n_hidden, hidden_size, output_size, input_name,
                                  label_name, error_name);

  // Initialise Optimiser
  Optimiser optimiser(g, {input_name}, label_name, error_name, learning_rate);

  state.counters["charge"] = static_cast<double>(optimiser.ChargeStep() * n_epochs);

  for (auto _ : state)
  {
    // Do optimisation
    for (SizeType i = 0; i < n_epochs; ++i)
    {
      optimiser.Run({data}, gt);
    }
  }
}

static void OptimiserRunArguments(benchmark::internal::Benchmark *b)
{
  std::vector<std::int64_t> batch_sizes{1, 10, 100};
  std::vector<std::int64_t> input_sizes{1, 10, 100};
  std::vector<std::int64_t> hidden_sizes{1, 10, 100};
  std::vector<std::int64_t> output_sizes{1, 10, 100};
  std::int64_t              n_epochs = 10;
  std::int64_t              n_hidden = 0;

  SizeType n_benchmarks = batch_sizes.size();
  for (SizeType i = 0; i < n_benchmarks; i++)
  {
    b->Args({batch_sizes[i], input_sizes[i], hidden_sizes[i], output_sizes[i], n_epochs, n_hidden});
  }
}

BENCHMARK_TEMPLATE(BM_Optimiser_Run,
                   fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<float>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Optimiser_Run,
                   fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<double>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Run,
    fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<fetch::fixed_point::fp32_t>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Run,
    fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<fetch::fixed_point::fp64_t>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Run,
    fetch::ml::optimisers::SGDOptimiser<fetch::math::Tensor<fetch::fixed_point::fp128_t>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Optimiser_Run,
                   fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<float>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Optimiser_Run,
                   fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<double>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Run,
    fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<fetch::fixed_point::fp32_t>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Run,
    fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<fetch::fixed_point::fp64_t>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(
    BM_Optimiser_Run,
    fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<fetch::fixed_point::fp128_t>>)
    ->Apply(OptimiserRunArguments)
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK_MAIN();
