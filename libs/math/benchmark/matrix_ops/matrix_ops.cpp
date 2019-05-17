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

#include <iostream>

#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"

#include "benchmark/benchmark.h"

template <class T, int C, int H, int W>
void BM_BooleanMaskEmpty(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;

  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<SizeType>{C, H, W});
    fetch::math::Tensor<T> mask(std::vector<SizeType>{C, H, W});
    mask.SetAllZero();
    state.ResumeTiming();

    fetch::math::BooleanMask(t, mask);
  }
}

BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskEmpty, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_BooleanMaskFull(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<SizeType>{C, H, W});
    fetch::math::Tensor<T> mask(std::vector<SizeType>{C, H, W});
    mask.SetAllOne();
    state.ResumeTiming();
    fetch::math::BooleanMask(t, mask);
  }
}

BENCHMARK_TEMPLATE(BM_BooleanMaskFull, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskFull, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_BooleanMaskFull, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_BooleanMaskFull, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int D, int H, int W>
void BM_Scatter3D(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<SizeType>{D, H, W});

    fetch::math::Tensor<T> updates(std::vector<SizeType>{D * W * H});
    updates.SetAllOne();

    std::vector<fetch::math::SizeVector> indices{};
    for (SizeType j = 0; j < D; ++j)
    {
      for (SizeType k = 0; k < H; ++k)
      {
        for (SizeType m = 0; m < W; ++m)
        {
          indices.emplace_back(fetch::math::SizeVector{j, k, m});
        }
      }
    }

    state.ResumeTiming();
    fetch::math::Scatter(t, updates, indices);
  }
}

BENCHMARK_TEMPLATE(BM_Scatter3D, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter3D, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter3D, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Scatter3D, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter3D, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter3D, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Scatter3D, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter3D, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter3D, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_Product(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    state.ResumeTiming();
    fetch::math::Product(t);
  }
}

BENCHMARK_TEMPLATE(BM_Product, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Product, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Product, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Product, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Product, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Product, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Product, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Product, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Product, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_Max(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    state.ResumeTiming();
    fetch::math::Max(t);
  }
}

BENCHMARK_TEMPLATE(BM_Max, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Max, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Max, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Max, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_MaxAxis(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    fetch::math::Tensor<T> ret(std::vector<std::uint64_t>{C, W});
    state.ResumeTiming();
    fetch::math::Max(t, SizeType(1), ret);
  }
}

BENCHMARK_TEMPLATE(BM_MaxAxis, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MaxAxis, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MaxAxis, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MaxAxis, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_Min(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    state.ResumeTiming();
    fetch::math::Min(t);
  }
}

BENCHMARK_TEMPLATE(BM_Min, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Min, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Min, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Min, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Min, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Min, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Min, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Min, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Min, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_MinAxis(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<std::uint64_t>{C, H, W});
    fetch::math::Tensor<T> ret(std::vector<std::uint64_t>{C, W});
    state.ResumeTiming();
    fetch::math::Min(t, SizeType(1), ret);
  }
}

BENCHMARK_TEMPLATE(BM_MinAxis, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MinAxis, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MinAxis, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MinAxis, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MinAxis, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MinAxis, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_MinAxis, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MinAxis, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MinAxis, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

template <class T, int C, int H, int W>
void BM_Maximum(benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t1(std::vector<std::uint64_t>{C, H, W});
    fetch::math::Tensor<T> t2(std::vector<std::uint64_t>{C, H, W});
    state.ResumeTiming();
    fetch::math::Maximum(t1, t2);
  }
}

BENCHMARK_TEMPLATE(BM_Maximum, int, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Maximum, float, 3, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Maximum, double, 3, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Maximum, int, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Maximum, float, 128, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Maximum, double, 128, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Maximum, int, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Maximum, float, 256, 256, 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Maximum, double, 256, 256, 256)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
