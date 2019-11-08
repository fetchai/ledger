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

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

template <typename T, fetch::math::SizeType B, fetch::math::SizeType S, fetch::math::SizeType D,
          fetch::math::SizeType E>
void BM_Setup_And_Train_Embeddings(benchmark::State &state)
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
    auto g = std::make_shared<fetch::ml::Graph<TensorType>>();

    // set up the neural net architecture
    std::string input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});
    std::string label_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});

    std::string output_name = g->template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
        "Embeddings", {input_name}, embedding_dimensions, n_datapoints);

    std::string error_name = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
        "", {output_name, label_name});

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

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 100, 1000000, 1)
->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 100, 1000000, 1)
->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 100, 1000000, 1)
->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 100, 1000000, 1)
->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 1000000, 1)
->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100000, 100, 1000000, 1)
->Unit(benchmark::kMillisecond);


/*
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 10, 10, 100000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 10, 10, 100000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 10, 10, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 10, 10, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 10, 10, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100000, 10, 10, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000000, 10, 10, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 100, 10, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 100, 10, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 100, 10, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 100, 10, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 10, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 10, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 1000, 10, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 1000, 10, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 1000, 10, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 1000, 10, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 1000, 10, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 10, 100, 100000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 10, 100, 100000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 10, 100, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 10, 100, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 10, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100000, 10, 100, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000000, 10, 100, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 100, 100, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 100, 100, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 100, 100, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 100, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 100, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 100, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 1000, 100, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 1000, 100, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 1000, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 1000, 100, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 1000, 100, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 10, 1000, 100000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 10, 1000, 100000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 10, 1000, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 10, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 10, 1000, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100000, 10, 1000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000000, 10, 1000, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 100, 1000, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 100, 1000, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 100, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 100, 1000, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 1000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 1000, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1, 1000, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10, 1000, 1000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 1000, 1000, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 1000, 1000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 1000, 1000, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 10, 10000, 10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 10, 10000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 10, 10000, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100000, 10, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000000, 10, 10000, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 100, 10000, 1000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 100, 10000, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 100, 10000, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 100, 1000, 10000, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 1000, 1000, 10000, 10)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train_Embeddings, float, 10000, 1000, 10000, 1)
    ->Unit(benchmark::kMillisecond);
*/

BENCHMARK_MAIN();
