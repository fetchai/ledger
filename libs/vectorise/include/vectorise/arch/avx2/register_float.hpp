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

#include "vectorise/arch/avx2/info.hpp"
#include "vectorise/info.hpp"
#include "vectorise/register.hpp"

#include <limits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <iomanip>

#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorise {

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
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm_load_ps(reinterpret_cast<type const *>(list.begin()));
  }
  
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm_set1_ps(c);
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

template <>
class VectorRegister<float, 256>
{
public:
  using type             = float;
  using mm_register_type = __m256;

  enum
  {
    E_VECTOR_SIZE   = 256,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() = default;
  VectorRegister(type const *d)
  {
    data_ = _mm256_load_ps(d);
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm256_load_ps(reinterpret_cast<type const *>(list.begin()));
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm256_set1_ps(c);
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm256_store_ps(ptr, data_);
  }
  void Stream(type *ptr) const
  {
    _mm256_stream_ps(ptr, data_);
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

template<>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<float, 128> const &n)
{
  alignas(16) float out[4];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<float>::digits10);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3];

  return s;
}

template<>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<float, 256> const &n)
{
  alignas(32) float out[8];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<float>::digits10);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3] << ", "
    << out[4] << ", " << out[5] << ", " << out[6] << ", " << out[7];

  return s;
}

inline VectorRegister<float, 128> operator-(VectorRegister<float, 128> const &x)
{
  return VectorRegister<float, 128>(_mm_sub_ps(_mm_setzero_ps(), x.data()));
}

inline VectorRegister<float, 256> operator-(VectorRegister<float, 256> const &x)
{
  return VectorRegister<float, 256>(_mm256_sub_ps(_mm256_setzero_ps(), x.data()));
}

#define FETCH_ADD_OPERATOR(op, type, size, L, fnc)                                       \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &a, \
                                               VectorRegister<type, size> const &b) \
  {                                                                                \
    L ret = fnc(a.data(), b.data());                                               \
    return VectorRegister<type, size>(ret);                                         \
  }

FETCH_ADD_OPERATOR(*, float, 128, __m128, _mm_mul_ps)
FETCH_ADD_OPERATOR(-, float, 128, __m128, _mm_sub_ps)
FETCH_ADD_OPERATOR(/, float, 128, __m128, _mm_div_ps)
FETCH_ADD_OPERATOR(+, float, 128, __m128, _mm_add_ps)

FETCH_ADD_OPERATOR(*, float, 256, __m256, _mm256_mul_ps)
FETCH_ADD_OPERATOR(-, float, 256, __m256, _mm256_sub_ps)
FETCH_ADD_OPERATOR(/, float, 256, __m256, _mm256_div_ps)
FETCH_ADD_OPERATOR(+, float, 256, __m256, _mm256_add_ps)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 128> operator op(VectorRegister<type, 128> const &a, \
                                               VectorRegister<type, 128> const &b) \
  {                                                                                \
    L ret  = fnc(a.data(), b.data());                                 \
    return VectorRegister<type, 128>(ret);                       \
  }

FETCH_ADD_OPERATOR(==, float, __m128, _mm_cmpeq_ps)
FETCH_ADD_OPERATOR(!=, float, __m128, _mm_cmpneq_ps)
FETCH_ADD_OPERATOR(>=, float, __m128, _mm_cmpge_ps)
FETCH_ADD_OPERATOR(>, float, __m128, _mm_cmpgt_ps)
FETCH_ADD_OPERATOR(<=, float, __m128, _mm_cmple_ps)
FETCH_ADD_OPERATOR(<, float, __m128, _mm_cmplt_ps)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 256> operator op(VectorRegister<type, 256> const &a, \
                                               VectorRegister<type, 256> const &b) \
  {                                                                                \
    L ret  = _mm256_cmp_ps(a.data(), b.data(), fnc);                                 \
    return VectorRegister<type, 256>(ret);                       \
  }

FETCH_ADD_OPERATOR(==, float, __m256, _CMP_EQ_OQ)
FETCH_ADD_OPERATOR(!=, float, __m256, _CMP_NEQ_UQ)
FETCH_ADD_OPERATOR(>=, float, __m256, _CMP_GE_OQ)
FETCH_ADD_OPERATOR(>, float, __m256, _CMP_GT_OQ)
FETCH_ADD_OPERATOR(<=, float, __m256, _CMP_LE_OQ)
FETCH_ADD_OPERATOR(<, float, __m256, _CMP_LT_OQ)

#undef FETCH_ADD_OPERATOR

// Note: Useful functions to manage NaN
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

inline VectorRegister<float, 256> vector_zero_below_element(VectorRegister<float, 256> const &a,
                                                            int const &                       n)
{
  alignas(32) const uint32_t mask[8] = {uint32_t(-(0 >= n)), uint32_t(-(1 >= n)),
                                        uint32_t(-(2 >= n)), uint32_t(-(3 >= n)),
                                        uint32_t(-(4 <= n)), uint32_t(-(5 <= n)),
                                        uint32_t(-(6 <= n)), uint32_t(-(7 <= n))};

  __m256i conv = _mm256_castps_si256(a.data());
  conv         = _mm256_and_si256(conv, *reinterpret_cast<__m256i const *>(mask));

  return VectorRegister<float, 256>(_mm256_castsi256_ps(conv));
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

inline VectorRegister<float, 256> vector_zero_above_element(VectorRegister<float, 256> const &a,
                                                            int const &                       n)
{
  alignas(32) const uint32_t mask[8] = {uint32_t(-(0 >= n)), uint32_t(-(1 >= n)),
                                        uint32_t(-(2 >= n)), uint32_t(-(3 >= n)),
                                        uint32_t(-(4 <= n)), uint32_t(-(5 <= n)),
                                        uint32_t(-(6 <= n)), uint32_t(-(7 <= n))};

  __m256i conv = _mm256_castps_si256(a.data());
  conv         = _mm256_and_si256(conv, *reinterpret_cast<__m256i const *>(mask));

  return VectorRegister<float, 256>(_mm256_castsi256_ps(conv));
}

inline VectorRegister<float, 128> shift_elements_left(VectorRegister<float, 128> const &x)
{
  __m128i n = _mm_castps_si128(x.data());
  n         = _mm_bslli_si128(n, 4);
  return VectorRegister<float, 128>(_mm_castsi128_ps(n));
}

inline VectorRegister<float, 256> shift_elements_left(VectorRegister<float, 256> const &x)
{
  __m256i n = _mm256_castps_si256(x.data());
  n         = _mm256_bslli_epi128(n, 4);
  return VectorRegister<float, 256>(_mm256_castsi256_ps(n));
}

inline VectorRegister<float, 128> shift_elements_right(VectorRegister<float, 128> const &x)
{
  __m128i n = _mm_castps_si128(x.data());
  n         = _mm_bsrli_si128(n, 4);
  return VectorRegister<float, 128>(_mm_castsi128_ps(n));
}

inline VectorRegister<float, 256> shift_elements_right(VectorRegister<float, 256> const &x)
{
  __m256i n = _mm256_castps_si256(x.data());
  n         = _mm256_bsrli_epi128(n, 4);
  return VectorRegister<float, 256>(_mm256_castsi256_ps(n));
}

inline float first_element(VectorRegister<float, 128> const &x)
{
  return _mm_cvtss_f32(x.data());
}

inline float first_element(VectorRegister<float, 256> const &x)
{
  return _mm256_cvtss_f32(x.data());
}

inline float reduce(VectorRegister<float, 128> const &x)
{
  __m128 r = _mm_hadd_ps(x.data(), _mm_setzero_ps());
  r        = _mm_hadd_ps(r, r);
  return _mm_cvtss_f32(r);
}

inline float reduce(VectorRegister<float, 256> const &x)
{
  __m256 r = _mm256_hadd_ps(x.data(), _mm256_setzero_ps());
  r        = _mm256_hadd_ps(r, _mm256_setzero_ps());
  r        = _mm256_hadd_ps(r, _mm256_setzero_ps());
  return _mm256_cvtss_f32(r);
}

inline bool all_less_than(VectorRegister<float, 128> const &x,
                          VectorRegister<float, 128> const &y)
{
  __m128i r = _mm_castps_si128((x < y).data());
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_less_than(VectorRegister<float, 256> const &x,
                          VectorRegister<float, 256> const &y)
{
  __m256i r = _mm256_castps_si256((x < y).data());
  return _mm256_movemask_epi8(r) == 0xFFFF;
}

inline bool any_less_than(VectorRegister<float, 128> const &x,
                          VectorRegister<float, 128> const &y)
{
  __m128i r = _mm_castps_si128((x < y).data());
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_less_than(VectorRegister<float, 256> const &x,
                          VectorRegister<float, 256> const &y)
{
  __m256i r = _mm256_castps_si256((x < y).data());
  return _mm256_movemask_epi8(r) != 0;
}

inline bool all_equal_to(VectorRegister<float, 128> const &x,
                          VectorRegister<float, 128> const &y)
{
  __m128i r = _mm_castps_si128((x == y).data());
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_equal_to(VectorRegister<float, 256> const &x,
                          VectorRegister<float, 256> const &y)
{
  __m256i r = _mm256_castps_si256((x == y).data());
  uint32_t mask = _mm256_movemask_epi8(r);
  return mask == 0xFFFFFFFFUL;
}

inline bool any_equal_to(VectorRegister<float, 128> const &x,
                          VectorRegister<float, 128> const &y)
{
  __m128i r = _mm_castps_si128((x == y).data());
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_equal_to(VectorRegister<float, 256> const &x,
                          VectorRegister<float, 256> const &y)
{
  __m256i r = _mm256_castps_si256((x == y).data());
  return _mm256_movemask_epi8(r) != 0;
}

}  // namespace vectorise
}  // namespace fetch
