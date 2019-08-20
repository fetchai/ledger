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
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

using SizeType = fetch::math::SizeType;
template <typename T>
void create_graph(std::shared_ptr<fetch::ml::Graph<typename fetch::math::Tensor<T>>> g,
                  SizeType input_size, SizeType output_size, SizeType n_layers)
{
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;
  // set up the neural net architecture
  std::string input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});

  for (int i = 0; i < static_cast<int>(n_layers); i++)
  {
    std::string h_1 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
        "FC_" + std::to_string(i), {input_name}, input_size, output_size);
  }
}

template <typename T, fetch::math::SizeType I, fetch::math::SizeType O, fetch::math::SizeType L>
void BM_Setup_And_Serialize(benchmark::State &state)
{
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  SizeType input_size  = I;
  SizeType output_size = O;
  SizeType n_layers    = L;

  // make a graph
  auto g = std::make_shared<fetch::ml::Graph<TensorType>>();
  create_graph<T>(g, input_size, output_size, n_layers);

  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g->GetGraphSaveableParams();

  for (auto _ : state)
  {
    fetch::serializers::MsgPackSerializer b;
    b << gsp1;
  }
}

BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 50, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 100, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 200, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 100, 2)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 100, 4)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 100, 8)->Unit(benchmark::kMillisecond);

template <typename T, fetch::math::SizeType I, fetch::math::SizeType O, fetch::math::SizeType L>
void BM_Setup_And_Deserialize(benchmark::State &state)
{
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  SizeType input_size  = I;
  SizeType output_size = O;
  SizeType n_layers    = L;

  // make a graph
  auto g = std::make_shared<fetch::ml::Graph<TensorType>>();
  create_graph<T>(g, input_size, output_size, n_layers);

  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g->GetGraphSaveableParams();
  fetch::serializers::MsgPackSerializer      b;
  b << gsp1;
  b.seek(0);

  for (auto _ : state)
  {
    fetch::ml::GraphSaveableParams<TensorType> gsp2;
    b >> gsp2;
    b.seek(0);
  }
}

BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, float, 500, 100, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, float, 500, 400, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, float, 500, 800, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, float, 500, 800, 2)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
