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

#include "math/base_types.hpp"
#include "math/tensor/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "benchmark/benchmark.h"

namespace {

struct BM_Tensor_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_Tensor_config(::benchmark::State const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(0));

    shape.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }
  }

  std::vector<SizeType> shape;  // layers input/output sizes
};

template <class T>
void BM_Divide_Tensors(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  // Get args from state
  BM_Tensor_config config{state};

  TensorType input_1(config.shape);
  TensorType input_2(config.shape);
  TensorType output(config.shape);

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();

  for (auto _ : state)
  {
    fetch::math::Divide(input_1, input_2, output);
  }
}

static void DivideTensorsArguments(benchmark::internal::Benchmark *b)
{
  using SizeType                   = fetch::math::SizeType;
  SizeType const N_ELEMENTS        = 2;
  std::int64_t   MAX_SIZE          = 2097152;
  std::int64_t   MAX_COMBINED_SIZE = 1024;

  std::vector<std::int64_t> dim_size;
  std::int64_t              i{1};
  while (i <= MAX_SIZE)
  {
    dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, j, 1});
  }
  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, 1, j});
  }

  std::vector<std::int64_t> combined_dim_size;
  i = 1;
  while (i <= MAX_COMBINED_SIZE)
  {
    combined_dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : combined_dim_size)
  {
    b->Args({N_ELEMENTS, j, j});
  }
}

BENCHMARK_TEMPLATE(BM_Divide_Tensors, fetch::fixed_point::fp64_t)
    ->Apply(DivideTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Tensors, float)
    ->Apply(DivideTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Tensors, double)
    ->Apply(DivideTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Tensors, fetch::fixed_point::fp32_t)
    ->Apply(DivideTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Tensors, fetch::fixed_point::fp128_t)
    ->Apply(DivideTensorsArguments)
    ->Unit(::benchmark::kNanosecond);

template <class T>
void BM_Divide_Scalar(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  using DataType   = typename TensorType::Type;

  // Get args from state
  BM_Tensor_config config{state};

  TensorType input_1(config.shape);
  DataType   input_2{2};
  TensorType output(config.shape);

  // Fill tensors with random values
  input_1.FillUniformRandom();

  for (auto _ : state)
  {
    fetch::math::Divide(input_1, input_2, output);
  }
}

static void DivideScalarArguments(benchmark::internal::Benchmark *b)
{
  using SizeType                   = fetch::math::SizeType;
  SizeType const N_ELEMENTS        = 2;
  std::int64_t   MAX_SIZE          = 2097152;
  std::int64_t   MAX_COMBINED_SIZE = 1024;

  std::vector<std::int64_t> dim_size;
  std::int64_t              i{1};
  while (i <= MAX_SIZE)
  {
    dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, j, 1});
  }
  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, 1, j});
  }

  std::vector<std::int64_t> combined_dim_size;
  i = 1;
  while (i <= MAX_COMBINED_SIZE)
  {
    combined_dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : combined_dim_size)
  {
    b->Args({N_ELEMENTS, j, j});
  }
}

BENCHMARK_TEMPLATE(BM_Divide_Scalar, fetch::fixed_point::fp64_t)
    ->Apply(DivideScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Scalar, float)
    ->Apply(DivideScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Scalar, double)
    ->Apply(DivideScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Scalar, fetch::fixed_point::fp32_t)
    ->Apply(DivideScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Divide_Scalar, fetch::fixed_point::fp128_t)
    ->Apply(DivideScalarArguments)
    ->Unit(::benchmark::kNanosecond);

template <class T>
void BM_Multiply_Tensors(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;

  // Get args from state
  BM_Tensor_config config{state};

  TensorType input_1(config.shape);
  TensorType input_2(config.shape);
  TensorType output(config.shape);

  // Fill tensors with random values
  input_1.FillUniformRandom();
  input_2.FillUniformRandom();

  for (auto _ : state)
  {
    fetch::math::Multiply(input_1, input_2, output);
  }
}

static void MultiplyTensorsArguments(benchmark::internal::Benchmark *b)
{
  using SizeType                   = fetch::math::SizeType;
  SizeType const N_ELEMENTS        = 2;
  std::int64_t   MAX_SIZE          = 2097152;
  std::int64_t   MAX_COMBINED_SIZE = 1024;

  std::vector<std::int64_t> dim_size;
  std::int64_t              i{1};
  while (i <= MAX_SIZE)
  {
    dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, j, 1});
  }
  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, 1, j});
  }

  std::vector<std::int64_t> combined_dim_size;
  i = 1;
  while (i <= MAX_COMBINED_SIZE)
  {
    combined_dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : combined_dim_size)
  {
    b->Args({N_ELEMENTS, j, j});
  }
}

BENCHMARK_TEMPLATE(BM_Multiply_Tensors, fetch::fixed_point::fp64_t)
    ->Apply(MultiplyTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Tensors, float)
    ->Apply(MultiplyTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Tensors, double)
    ->Apply(MultiplyTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Tensors, fetch::fixed_point::fp32_t)
    ->Apply(MultiplyTensorsArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Tensors, fetch::fixed_point::fp128_t)
    ->Apply(MultiplyTensorsArguments)
    ->Unit(::benchmark::kNanosecond);

template <class T>
void BM_Multiply_Scalar(benchmark::State &state)
{
  using TensorType = typename fetch::math::Tensor<T>;
  using DataType   = typename TensorType::Type;

  // Get args from state
  BM_Tensor_config config{state};

  TensorType input_1(config.shape);
  DataType   input_2{2};
  TensorType output(config.shape);

  // Fill tensors with random values
  input_1.FillUniformRandom();

  for (auto _ : state)
  {
    fetch::math::Multiply(input_1, input_2, output);
  }
}

static void MultiplyScalarArguments(benchmark::internal::Benchmark *b)
{
  using SizeType                   = fetch::math::SizeType;
  SizeType const N_ELEMENTS        = 2;
  std::int64_t   MAX_SIZE          = 2097152;
  std::int64_t   MAX_COMBINED_SIZE = 1024;

  std::vector<std::int64_t> dim_size;
  std::int64_t              i{1};
  while (i <= MAX_SIZE)
  {
    dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, j, 1});
  }
  for (std::int64_t &j : dim_size)
  {
    b->Args({N_ELEMENTS, 1, j});
  }

  std::vector<std::int64_t> combined_dim_size;
  i = 1;
  while (i <= MAX_COMBINED_SIZE)
  {
    combined_dim_size.push_back(i);
    i *= 2;
  }

  for (std::int64_t &j : combined_dim_size)
  {
    b->Args({N_ELEMENTS, j, j});
  }
}

BENCHMARK_TEMPLATE(BM_Multiply_Scalar, fetch::fixed_point::fp64_t)
    ->Apply(MultiplyScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Scalar, float)
    ->Apply(MultiplyScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Scalar, double)
    ->Apply(MultiplyScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Scalar, fetch::fixed_point::fp32_t)
    ->Apply(MultiplyScalarArguments)
    ->Unit(::benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_Multiply_Scalar, fetch::fixed_point::fp128_t)
    ->Apply(MultiplyScalarArguments)
    ->Unit(::benchmark::kNanosecond);

}  // namespace
