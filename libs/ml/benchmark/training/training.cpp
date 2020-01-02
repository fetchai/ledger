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
#include "ml/optimisation/sgd_optimiser.hpp"

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

template <typename T, fetch::math::SizeType B, fetch::math::SizeType I, fetch::math::SizeType H,
          fetch::math::SizeType O, fetch::math::SizeType E>
void BM_Setup_And_Train(benchmark::State &state)
{
  using SizeType   = fetch::math::SizeType;
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  SizeType batch_size  = B;
  SizeType input_size  = I;
  SizeType hidden_size = H;
  SizeType output_size = O;
  SizeType n_epochs    = E;

  auto learning_rate = fetch::math::Type<DataType>("0.1");

  // Prepare data and labels
  TensorType data({input_size, batch_size});
  TensorType gt({output_size, batch_size});
  data.FillUniformRandom();
  gt.FillUniformRandom();

  for (auto _ : state)
  {
    // make a graph
    auto g = std::make_shared<fetch::ml::Graph<TensorType>>();

    // set up the neural net architecture
    std::string input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});
    std::string label_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});

    std::string h_1 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
        "FC1", {input_name}, input_size, hidden_size);
    std::string a_1 = g->template AddNode<fetch::ml::ops::Relu<TensorType>>("", {h_1});

    std::string h_2 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
        "FC2", {a_1}, hidden_size, output_size);
    std::string output_name = g->template AddNode<fetch::ml::ops::Relu<TensorType>>("", {h_2});

    std::string error_name = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
        "", {output_name, label_name});

    // Initialise Optimiser
    fetch::ml::optimisers::SGDOptimiser<TensorType> optimiser(g, {input_name}, label_name,
                                                              error_name, learning_rate);

    // Do optimisation
    for (SizeType i = 0; i < n_epochs; ++i)
    {
      optimiser.Run({data}, gt);
    }
  }
}

BENCHMARK_TEMPLATE(BM_Setup_And_Train, float, 1, 1, 1, 1, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train, float, 10, 10, 10, 10, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train, float, 100, 100, 100, 100, 100)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Train, float, 100, 1000, 1000, 1000, 100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
