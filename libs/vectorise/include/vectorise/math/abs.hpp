#pragma once

#include "vectorise/sse.hpp"

#include <emmintrin.h>
#include <limits>

namespace fetch {
namespace vectorize {

inline VectorRegister<float, 128> abs(VectorRegister<float, 128> const &a)
{
  const __m128 sign = _mm_castsi128_ps(_mm_set1_epi32(1 << 31));
  return VectorRegister<float, 128>(_mm_andnot_ps(sign, a.data()));
}

inline VectorRegister<double, 128> abs(VectorRegister<double, 128> const &a)
{
  const __m128d mask = _mm_castsi128_pd(_mm_set1_epi64x(std::numeric_limits<int64_t>::max()));
  return VectorRegister<double, 128>(_mm_and_pd(mask, a.data()));
}

}  // namespace vectorize
}  // namespace fetch
