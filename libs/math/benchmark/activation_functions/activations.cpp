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

#include "math/activation_functions/elu.hpp"
#include "math/activation_functions/leaky_relu.hpp"
#include "math/activation_functions/relu.hpp"
#include "math/activation_functions/sigmoid.hpp"
#include "math/activation_functions/softmax.hpp"

#include "benchmark/benchmark.h"
#include "math/tensor.hpp"

using namespace fetch::math;

template <typename T, SizeType L, SizeType H, SizeType W>
void BM_Elu(benchmark::State &state)
{
  Tensor<T> input({L, H, W});
  Tensor<T> output({L, H, W});
  T         a = T(0.2);

  for (auto _ : state)
  {
    Elu<Tensor<T>>(input, a, output);
  }
}

BENCHMARK_TEMPLATE(BM_Elu, float, 2, 4, 16)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Elu, double, 2, 4, 16)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Elu, float, 2, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Elu, double, 2, 8, 128)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Elu, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Elu, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <typename T, SizeType L, SizeType H, SizeType W>
void BM_Relu(benchmark::State &state)
{
  Tensor<T> input({L, H, W});
  Tensor<T> output({L, H, W});

  for (auto _ : state)
  {
    Relu<Tensor<T>>(input, output);
  }
}

BENCHMARK_TEMPLATE(BM_Relu, int, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Relu, float, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Relu, double, 2, 2, 2)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Relu, int, 2, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Relu, float, 2, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Relu, double, 2, 8, 128)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Relu, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Relu, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Relu, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <typename T, SizeType L, SizeType H, SizeType W>
void BM_LeakyRelu(benchmark::State &state)
{
  Tensor<T> input({L, H, W});
  Tensor<T> output({L, H, W});

  for (auto _ : state)
  {
    LeakyRelu<Tensor<T>>(input, output);
  }
}

BENCHMARK_TEMPLATE(BM_LeakyRelu, int, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyRelu, float, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyRelu, double, 2, 2, 2)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LeakyRelu, int, 2, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyRelu, float, 2, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_LeakyRelu, double, 2, 8, 128)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_LeakyRelu, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_LeakyRelu, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_LeakyRelu, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <typename T, SizeType L, SizeType H, SizeType W>
void BM_Sigmoid(benchmark::State &state)
{
  Tensor<T> input({L, H, W});
  Tensor<T> output({L, H, W});

  for (auto _ : state)
  {
    Sigmoid<Tensor<T>>(input, output);
  }
}

BENCHMARK_TEMPLATE(BM_Sigmoid, int, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Sigmoid, float, 2, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Sigmoid, double, 2, 2, 2)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Sigmoid, int, 2, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Sigmoid, float, 2, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Sigmoid, double, 2, 8, 128)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Sigmoid, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Sigmoid, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Sigmoid, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <typename T, SizeType L, SizeType H>
void BM_Softmax(benchmark::State &state)
{
  Tensor<T> input({L, H});
  Tensor<T> output({L, H});

  for (auto _ : state)
  {
    Softmax<Tensor<T>>(input, output);
  }
}

BENCHMARK_TEMPLATE(BM_Softmax, int, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Softmax, float, 2, 2)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Softmax, double, 2, 2)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Softmax, int, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Softmax, float, 8, 128)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Softmax, double, 8, 128)->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(BM_Softmax, int, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Softmax, float, 256, 256)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Softmax, double, 256, 256)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
