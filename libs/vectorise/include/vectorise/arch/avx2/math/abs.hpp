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

#include <emmintrin.h>
#include <limits>

namespace fetch {
namespace vectorise {

inline VectorRegister<int32_t, 128> Abs(VectorRegister<int32_t, 128> const &a)
{
  auto const ret = VectorRegister<int32_t, 128>(_mm_abs_epi32(a.data()));
  return ret;
}

inline VectorRegister<int32_t, 256> Abs(VectorRegister<int32_t, 256> const &a)
{
  auto const ret = VectorRegister<int32_t, 256>(_mm256_abs_epi32(a.data()));
  return ret;
}

inline VectorRegister<int64_t, 128> Abs(VectorRegister<int64_t, 128> const &a)
{
  auto const ret  = VectorRegister<int64_t, 128>(_mm_abs_epi64(a.data()));
  return ret;
}

inline VectorRegister<int64_t, 256> Abs(VectorRegister<int64_t, 256> const &a)
{
  auto const ret  = VectorRegister<int64_t, 256>(_mm256_abs_epi64(a.data()));
  return ret;
}

inline VectorRegister<fixed_point::fp32_t, 128> Abs(VectorRegister<fixed_point::fp32_t, 128> const &a)
{
  VectorRegister<int32_t, 128> abs_int32 = Abs(VectorRegister<int32_t, 128>(a.data()));
  return {abs_int32.data()};
}

inline VectorRegister<fixed_point::fp32_t, 256> Abs(VectorRegister<fixed_point::fp32_t, 256> const &a)
{
  VectorRegister<int32_t, 256> abs_int32 = Abs(VectorRegister<int32_t, 256>(a.data()));
  return {abs_int32.data()};
}

inline VectorRegister<fixed_point::fp64_t, 128> Abs(VectorRegister<fixed_point::fp64_t, 128> const &a)
{
  VectorRegister<int64_t, 128> abs_int64 = Abs(VectorRegister<int64_t, 128>(a.data()));
  return {abs_int64.data()};
}

inline VectorRegister<fixed_point::fp64_t, 256> Abs(VectorRegister<fixed_point::fp64_t, 256> const &a)
{
  VectorRegister<int64_t, 256> abs_int64 = Abs(VectorRegister<int64_t, 256>(a.data()));
  return {abs_int64.data()};
}

inline VectorRegister<float, 128> Abs(VectorRegister<float, 128> const &a)
{
  const __m128 sign = _mm_castsi128_ps(_mm_set1_epi32(std::numeric_limits<int32_t>::max()));
  return {_mm_andnot_ps(sign, a.data())};
}

inline VectorRegister<float, 256> Abs(VectorRegister<float, 256> const &a)
{
  const __m256 sign = _mm256_castsi256_ps(_mm256_set1_epi32(std::numeric_limits<int32_t>::max()));
  return {_mm256_andnot_ps(sign, a.data())};
}

inline VectorRegister<double, 128> Abs(VectorRegister<double, 128> const &a)
{
  const __m128d mask = _mm_castsi128_pd(_mm_set1_epi64x(std::numeric_limits<int64_t>::max()));
  return {_mm_and_pd(mask, a.data())};
}

inline VectorRegister<double, 256> Abs(VectorRegister<double, 256> const &a)
{
  const __m256d mask = _mm256_castsi256_pd(_mm256_set1_epi64x(std::numeric_limits<int64_t>::max()));
  return {_mm256_and_pd(mask, a.data())};
}

}  // namespace vectorise
}  // namespace fetch
