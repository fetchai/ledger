#pragma once
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

#include "vectorise/vectorise.hpp"
#ifdef __AVX2__
#include "vectorise/arch/avx2/math/max.hpp"
#endif

#include <cmath>
#include <cstddef>

namespace fetch {
namespace vectorise {

template <typename T>
inline fetch::math::meta::IfIsNonFixedPointArithmetic<T, T> Max(T const &a, T const &b)
{
  return T(std::max(a, b));
}

template <typename T>
inline fetch::math::meta::IfIsFixedPoint<T, T> Max(T const &a, T const &b)
{
  return T::FromBase(std::max(a.Data(), b.Data()));
}

template <typename T>
inline VectorRegister<T, 8 * sizeof(T)> Max(VectorRegister<T, 8 * sizeof(T)> const &a,
                                            VectorRegister<T, 8 * sizeof(T)> const &b)
{
  return VectorRegister<T, 8 * sizeof(T)>(fetch::vectorise::Max(a.data(), b.data()));
}

template <typename T, std::size_t N>
inline T Max(VectorRegister<T, N> const &a)
{

  constexpr std::size_t                            size = N / (8 * sizeof(T));
  alignas(VectorRegister<T, N>::E_REGISTER_SIZE) T A[size];
  a.Store(A);
  T max{A[0]};
  for (std::size_t i = 1; i < size; i++)
  {
    max = Max(A[i], max);
  }
  return max;
}

}  // namespace vectorise
}  // namespace fetch
