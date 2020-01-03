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

#include "logging/logging.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/lazy_adam_optimiser.hpp"

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

/**
 *  Point of this benchmark is to get an idea of the overall time cost of training a simple
 * embeddings model under different hyperparameters and to compare performance benefits of sparse
 * optimisers with normal optimisers
 */

namespace fetch {
namespace ml {
namespace benchmark {

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

template <typename T, typename OptimiserType>
void BM_Setup_And_Train_Embeddings(::benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  auto batch_size           = static_cast<SizeType>(state.range(0));
  auto embedding_dimensions = static_cast<SizeType>(state.range(1));
  auto n_datapoints         = static_cast<SizeType>(state.range(2));
  auto n_epochs             = static_cast<SizeType>(state.range(3));
  auto learning_rate        = fetch::math::Type<DataType>("0.1");

  // Prepare data and labels
  TensorType data({1, batch_size});
  TensorType gt({embedding_dimensions, 1, batch_size});
  data.FillUniformRandomIntegers(0, static_cast<int64_t>(n_datapoints));
  gt.FillUniformRandom();

  // make a graph
  std::string input_name;
  std::string label_name;
  std::string error_name;
  auto g = PrepareTestGraph<TensorType>(embedding_dimensions, n_datapoints, input_name, label_name,
                                        error_name);

  // Initialise Optimiser
  OptimiserType optimiser(g, {input_name}, label_name, error_name, learning_rate);

  for (auto _ : state)
  {
    // Do optimisation
    for (SizeType i = 0; i < n_epochs; ++i)
    {
      state.PauseTiming();
      data.FillUniformRandomIntegers(0, static_cast<int64_t>(n_datapoints));
      state.ResumeTiming();
      ::benchmark::DoNotOptimize(optimiser.Run({data}, gt));
    }
  }
}

// Define all test cases with different parameters
static void CustomArguments(::benchmark::internal::Benchmark *b)
{

  // 1,10,100,1000,10000
  int num = 1;
  for (int i = 0; i < 4; ++i)
  {
    b->Args({num, 500, 10000, 10});
    num = num * 10;
  }

  // 20000,30000,40000,50000,60000,70000,80000,90000,100000
  for (int i = 0; i < 10; ++i)
  {
    b->Args({num, 500, 10000, 10});
    num += 10000;
  }
}

// Normal Adam tests
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float,
                   fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<float>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, double,
                   fetch::ml::optimisers::AdamOptimiser<fetch::math::Tensor<double>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, fetch::fixed_point::FixedPoint<16, 16>,
                   fetch::ml::optimisers::AdamOptimiser<
                       fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, fetch::fixed_point::FixedPoint<32, 32>,
                   fetch::ml::optimisers::AdamOptimiser<
                       fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

// Sparse LazyAdam tests
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float,
                   fetch::ml::optimisers::LazyAdamOptimiser<fetch::math::Tensor<float>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, double,
                   fetch::ml::optimisers::LazyAdamOptimiser<fetch::math::Tensor<double>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, fetch::fixed_point::FixedPoint<16, 16>,
                   fetch::ml::optimisers::LazyAdamOptimiser<
                       fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, fetch::fixed_point::FixedPoint<32, 32>,
                   fetch::ml::optimisers::LazyAdamOptimiser<
                       fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>)
    ->Apply(CustomArguments)
    ->Unit(::benchmark::kMillisecond);

}  // namespace benchmark
}  // namespace ml
}  // namespace fetch

BENCHMARK_MAIN();
