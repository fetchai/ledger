#pragma once
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

#include "vectorise/arch/avx2.hpp"

#include <emmintrin.h>
#include <limits>

namespace fetch {
namespace vectorise {

inline VectorRegister<float, 256> abs(VectorRegister<float, 256> const &a)
{
  const __m256 sign = _mm256_castsi256_ps(_mm256_set1_epi32(1 << 31));
  return VectorRegister<float, 256>(_mm256_andnot_ps(sign, a.data()));
}

inline VectorRegister<float, 128> abs(VectorRegister<float, 128> const &a)
{
  const __m128 sign = _mm_castsi128_ps(_mm_set1_epi32(1 << 31));
  return VectorRegister<float, 128>(_mm_andnot_ps(sign, a.data()));
}

inline VectorRegister<double, 256> abs(VectorRegister<double, 256> const &a)
{
  const __m256d mask = _mm256_castsi256_pd(_mm256_set1_epi64x(std::numeric_limits<int64_t>::max()));
  return VectorRegister<double, 256>(_mm256_and_pd(mask, a.data()));
}

inline VectorRegister<double, 128> abs(VectorRegister<double, 128> const &a)
{
  const __m128d mask = _mm_castsi128_pd(_mm_set1_epi64x(std::numeric_limits<int64_t>::max()));
  return VectorRegister<double, 128>(_mm_and_pd(mask, a.data()));
}

}  // namespace vectorise
}  // namespace fetch
