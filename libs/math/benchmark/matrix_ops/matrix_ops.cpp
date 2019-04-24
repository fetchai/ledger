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

#include <iomanip>
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

template <class T, int S>
void BM_Scatter1D(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<SizeType>{S});

    fetch::math::Tensor<T> updates(std::vector<SizeType>{S});
    updates.SetAllOne();

    std::vector<SizeType> indices{};
    for (std::size_t j = 0; j < S; ++j)
    {
      indices.emplace_back(j);
    }

    state.ResumeTiming();
    fetch::math::Scatter1D(t, updates, indices);
  }
}

BENCHMARK_TEMPLATE(BM_Scatter1D, int, 3 * 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter1D, float, 3 * 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter1D, double, 3 * 256 * 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Scatter1D, int, 128 * 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter1D, float, 128 * 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter1D, double, 128 * 256 * 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Scatter1D, int, 256 * 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter1D, float, 256 * 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter1D, double, 256 * 256 * 256)->Unit(benchmark::kMillisecond);

template <class T, int D, int W>
void BM_Scatter2D(benchmark::State &state)
{
  using SizeType = fetch::math::SizeType;
  for (auto _ : state)
  {
    state.PauseTiming();
    fetch::math::Tensor<T> t(std::vector<SizeType>{D, W});

    fetch::math::Tensor<T> updates(std::vector<SizeType>{D * W});
    updates.SetAllOne();

    std::vector<SizeType> indices_0{};
    std::vector<SizeType> indices_1{};
    for (SizeType j = 0; j < D; ++j)
    {
      for (SizeType k = 0; k < W; ++k)
      {
        indices_0.emplace_back(j);
        indices_1.emplace_back(k);
      }
    }

    state.ResumeTiming();
    fetch::math::Scatter2D(t, updates, indices_0, indices_1);
  }
}

BENCHMARK_TEMPLATE(BM_Scatter2D, int, 3, 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter2D, float, 3, 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter2D, double, 3, 256 * 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Scatter2D, int, 128, 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter2D, float, 128, 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter2D, double, 128, 256 * 256)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Scatter2D, int, 256, 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter2D, float, 256, 256 * 256)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Scatter2D, double, 256, 256 * 256)->Unit(benchmark::kMillisecond);

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

    std::vector<SizeType> indices_0{};
    std::vector<SizeType> indices_1{};
    std::vector<SizeType> indices_2{};
    for (SizeType j = 0; j < D; ++j)
    {
      for (SizeType k = 0; k < H; ++k)
      {
        for (SizeType m = 0; m < W; ++m)
        {
          indices_0.emplace_back(j);
          indices_1.emplace_back(k);
          indices_2.emplace_back(m);
        }
      }
    }

    state.ResumeTiming();
    fetch::math::Scatter3D(t, updates, indices_0, indices_1, indices_2);
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

BENCHMARK_MAIN();
