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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"

#include "benchmark/benchmark.h"

#include <memory>
#include <string>

using SizeType = fetch::math::SizeType;
template <typename T>
void CreateGraph(std::shared_ptr<fetch::ml::Graph<typename fetch::math::Tensor<T>>> g,
                 SizeType dims, SizeType n_layers)
{
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;
  // set up the neural net architecture
  std::string input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("", {});

  std::string h_1 = input_name;
  for (int i = 0; i < static_cast<int>(n_layers); i++)
  {
    h_1 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
        "FC_No_" + std::to_string(i), {h_1}, dims, dims);
  }
}

template <typename T, fetch::math::SizeType D, fetch::math::SizeType L>
void BM_Setup_And_Serialize(benchmark::State &state)
{
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  SizeType dims     = D;
  SizeType n_layers = L;

  // make a graph
  auto g = std::make_shared<fetch::ml::Graph<TensorType>>();
  CreateGraph<T>(g, dims, n_layers);

  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g->GetGraphSaveableParams();

  fetch::serializers::MsgPackSerializer b;

  for (auto _ : state)
  {
    b << gsp1;
  }
}

BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 200, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 2)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 4)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, float, 100, 8)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, double, 100, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, double, 200, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, double, 100, 2)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, double, 100, 4)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, double, 100, 8)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp32_t, 100, 1)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp32_t, 200, 1)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp32_t, 100, 2)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp32_t, 100, 4)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp32_t, 100, 8)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp64_t, 100, 1)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp64_t, 200, 1)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp64_t, 100, 2)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp64_t, 100, 4)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Serialize, fetch::fixed_point::fp64_t, 100, 8)
    ->Unit(benchmark::kMillisecond);
template <typename T, fetch::math::SizeType D, fetch::math::SizeType L>
void BM_Setup_And_Deserialize(benchmark::State &state)
{
  using DataType   = T;
  using TensorType = fetch::math::Tensor<DataType>;

  SizeType dims     = D;
  SizeType n_layers = L;

  // make a graph
  auto g = std::make_shared<fetch::ml::Graph<TensorType>>();
  CreateGraph<T>(g, dims, n_layers);

  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g->GetGraphSaveableParams();
  fetch::serializers::MsgPackSerializer      b;
  b << gsp1;
  b.seek(0);

  fetch::ml::GraphSaveableParams<TensorType> gsp2;

  for (auto _ : state)
  {
    benchmark::DoNotOptimize(b >> gsp2);
    b.seek(0);
  }
}

BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, float, 100, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, float, 200, 1)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, double, 100, 1)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, double, 200, 1)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, fetch::fixed_point::fp32_t, 100, 1)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, fetch::fixed_point::fp32_t, 200, 1)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, fetch::fixed_point::fp64_t, 100, 1)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Setup_And_Deserialize, fetch::fixed_point::fp64_t, 200, 1)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_MAIN();
