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

#include "vectorise/arch/sse/info.hpp"
#include "vectorise/arch/sse/register_int32.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

#include <iostream>

namespace fetch {
namespace vectorize {

template <>
class VectorRegister<float, 128>
{
public:
  using type             = float;
  using mm_register_type = __m128;

  enum
  {
    E_VECTOR_SIZE   = 128,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() = default;
  VectorRegister(type const *d)
  {
    data_ = _mm_load_ps(d);
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm_load_ps1(&c);
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm_store_ps(ptr, data_);
  }
  void Stream(type *ptr) const
  {
    _mm_stream_ps(ptr, data_);
  }

  mm_register_type const &data() const
  {
    return data_;
  }
  mm_register_type &data()
  {
    return data_;
  }

private:
  mm_register_type data_;
};

#define FETCH_ADD_OPERATOR(zero, type, fnc)                                      \
  inline VectorRegister<type, 128> operator-(VectorRegister<type, 128> const &x) \
  {                                                                              \
    return VectorRegister<type, 128>(fnc(zero(), x.data()));                     \
  }

FETCH_ADD_OPERATOR(_mm_setzero_ps, float, _mm_sub_ps)
#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 128> operator op(VectorRegister<type, 128> const &a, \
                                               VectorRegister<type, 128> const &b) \
  {                                                                                \
    L ret = fnc(a.data(), b.data());                                               \
    return VectorRegister<type, 128>(ret);                                         \
  }

FETCH_ADD_OPERATOR(*, float, __m128, _mm_mul_ps)
FETCH_ADD_OPERATOR(-, float, __m128, _mm_sub_ps)
FETCH_ADD_OPERATOR(/, float, __m128, _mm_div_ps)
FETCH_ADD_OPERATOR(+, float, __m128, _mm_add_ps)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 128> operator op(VectorRegister<type, 128> const &a, \
                                               VectorRegister<type, 128> const &b) \
  {                                                                                \
    L              imm  = fnc(a.data(), b.data());                                 \
    __m128i        ival = _mm_castps_si128(imm);                                   \
    constexpr type done = type(1);                                                 \
    const __m128i  one  = _mm_castps_si128(_mm_load_ps1(&done));                   \
    __m128i        ret  = _mm_and_si128(ival, one);                                \
    return VectorRegister<type, 128>(_mm_castsi128_ps(ret));                       \
  }

FETCH_ADD_OPERATOR(==, float, __m128, _mm_cmpeq_ps)
FETCH_ADD_OPERATOR(!=, float, __m128, _mm_cmpneq_ps)
FETCH_ADD_OPERATOR(>=, float, __m128, _mm_cmpge_ps)
FETCH_ADD_OPERATOR(>, float, __m128, _mm_cmpgt_ps)
FETCH_ADD_OPERATOR(<=, float, __m128, _mm_cmple_ps)
FETCH_ADD_OPERATOR(<, float, __m128, _mm_cmplt_ps)

#undef FETCH_ADD_OPERATOR

// Manage NaN
//__m128d _mm_cmpord_pd (__m128d a, __m128d b)
//__m128d _mm_cmpunord_pd (__m128d a, __m128d b)

// Floats
inline VectorRegister<float, 128> vector_zero_below_element(VectorRegister<float, 128> const &a,
                                                            int const &                       n)
{
  alignas(16) const uint32_t mask[4] = {uint32_t(-(0 >= n)), uint32_t(-(1 >= n)),
                                        uint32_t(-(2 >= n)), uint32_t(-(3 >= n))};

  __m128i conv = _mm_castps_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i const *>(mask));

  return VectorRegister<float, 128>(_mm_castsi128_ps(conv));
}

inline VectorRegister<float, 128> vector_zero_above_element(VectorRegister<float, 128> const &a,
                                                            int const &                       n)
{
  alignas(16) const uint32_t mask[4] = {uint32_t(-(0 <= n)), uint32_t(-(1 <= n)),
                                        uint32_t(-(2 <= n)), uint32_t(-(3 <= n))};

  __m128i conv = _mm_castps_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i const *>(mask));

  return VectorRegister<float, 128>(_mm_castsi128_ps(conv));
}

inline VectorRegister<float, 128> shift_elements_left(VectorRegister<float, 128> const &x)
{
  __m128i n = _mm_castps_si128(x.data());
  n         = _mm_bslli_si128(n, 4);
  return VectorRegister<float, 128>(_mm_castsi128_ps(n));
}

inline VectorRegister<float, 128> shift_elements_right(VectorRegister<float, 128> const &x)
{
  __m128i n = _mm_castps_si128(x.data());
  n         = _mm_bsrli_si128(n, 4);
  return VectorRegister<float, 128>(_mm_castsi128_ps(n));
}

inline float first_element(VectorRegister<float, 128> const &x)
{
  return _mm_cvtss_f32(x.data());
}

inline float reduce(VectorRegister<float, 128> const &x)
{
  __m128 r = _mm_hadd_ps(x.data(), _mm_setzero_ps());
  r        = _mm_hadd_ps(r, _mm_setzero_ps());
  return _mm_cvtss_f32(r);
}

/*
// TODO: Float equivalent
inline bool all_less_than(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128(_mm_cmplt_pd(x.data(), y.data()));
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool any_less_than(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128(_mm_cmplt_pd(x.data(), y.data()));
  return _mm_movemask_epi8(r) != 0;
}
*/

}  // namespace vectorize
}  // namespace fetch