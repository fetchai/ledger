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

#ifdef __AVX__
#include "vectorise/info.hpp"
#include "vectorise/info_avx.hpp"
#include "vectorise/register.hpp"

#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

// ???
#include "vectorise/sse.hpp"

namespace fetch {
namespace vectorize {

// SSE integers
template <typename T>
class VectorRegister<T, 256>
{
public:
  using type             = T;
  using mm_register_type = __m256i;

  enum
  {
    E_VECTOR_SIZE   = 256,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister()
  {}
  VectorRegister(type const *d)
  {
    data_ = _mm256_load_si256((mm_register_type *)d);
  }
  VectorRegister(type const &c)
  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet<type, E_BLOCK_COUNT>::Set(constant, c);
    data_ = _mm256_load_si256((mm_register_type *)constant);
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm256_store_si256((mm_register_type *)ptr, data_);
  }
  void Stream(type *ptr) const
  {
    _mm256_stream_si256((mm_register_type *)ptr, data_);
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

  VectorRegister()
  {}
  VectorRegister(type const *d)
  {
    data_ = _mm256_load_ps(d);
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet<type, E_BLOCK_COUNT>::Set(constant, c);
    data_ = _mm256_load_ps(constant);
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

template <>
class VectorRegister<double, 256>
{
public:
  using type             = double;
  using mm_register_type = __m256d;

  enum
  {
    E_VECTOR_SIZE   = 256,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister()
  {}
  VectorRegister(type const *d)
  {
    data_ = _mm256_load_pd(d);
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet<type, E_BLOCK_COUNT>::Set(constant, c);
    data_ = _mm256_load_pd(constant);
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm256_stream_pd(ptr, data_);
  }
  void Stream(type *ptr) const
  {
    _mm256_stream_pd(ptr, data_);
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

#define FETCH_ADD_OPERATOR(op, type, fnc)                                          \
  inline VectorRegister<type, 256> operator op(VectorRegister<type, 256> const &a, \
                                               VectorRegister<type, 256> const &b) \
  {                                                                                \
    VectorRegister<type, 256>::mm_register_type ret = fnc(a.data(), b.data());     \
    return VectorRegister<type, 256>(ret);                                         \
  }

#ifdef __AVX2__
FETCH_ADD_OPERATOR(*, int, _mm256_mullo_epi32);
FETCH_ADD_OPERATOR(-, int, _mm256_sub_epi32);
// FETCH_ADD_OPERATOR(/, int, __m256i, _mm256_div_epi32);
FETCH_ADD_OPERATOR(+, int, _mm256_add_epi32);

FETCH_ADD_OPERATOR(==, int, _mm256_cmpeq_epi32)
#endif

FETCH_ADD_OPERATOR(*, float, _mm256_mul_ps);
FETCH_ADD_OPERATOR(-, float, _mm256_sub_ps);
FETCH_ADD_OPERATOR(/, float, _mm256_div_ps);
FETCH_ADD_OPERATOR(+, float, _mm256_add_ps);

FETCH_ADD_OPERATOR(*, double, _mm256_mul_pd);
FETCH_ADD_OPERATOR(-, double, _mm256_sub_pd);
FETCH_ADD_OPERATOR(/, double, _mm256_div_pd);
FETCH_ADD_OPERATOR(+, double, _mm256_add_pd);

#undef FETCH_ADD_OPERATOR

// TODO(issue 4): Misses alignas
#define FETCH_ADD_OPERATOR(op, type, fnc)                                                           \
  inline VectorRegister<type, 256> operator op(VectorRegister<type, 256> const &a,                  \
                                               VectorRegister<type, 256> const &b)                  \
  {                                                                                                 \
    VectorRegister<type, 256>::mm_register_type imm  = _mm256_cmp_ps(a.data(), b.data(), fnc);      \
    __m256i                                     ival = _mm256_castps_si256(imm);                    \
    type                 done[VectorRegister<type, 256>::E_BLOCK_COUNT] = {1.0f, 1.0f, 1.0f, 1.0f,  \
                                                           1.0f, 1.0f, 1.0f, 1.0f}; \
    static const __m256i one = _mm256_castps_si256(_mm256_load_ps(done));                           \
    __m256i              ret = _mm256_and_si256(ival, one);                                         \
    return VectorRegister<type, 256>(_mm256_castsi256_ps(ret));                                     \
  }

FETCH_ADD_OPERATOR(==, float, _CMP_EQ_OQ)
FETCH_ADD_OPERATOR(!=, float, _CMP_NEQ_OQ)
FETCH_ADD_OPERATOR(>=, float, _CMP_GE_OQ)
FETCH_ADD_OPERATOR(>, float, _CMP_GT_OQ)
FETCH_ADD_OPERATOR(<=, float, _CMP_LE_OQ)
FETCH_ADD_OPERATOR(<, float, _CMP_LT_OQ)

#undef FETCH_ADD_OPERATOR

// TODO(issue 4): Misses alignas
#define FETCH_ADD_OPERATOR(op, type, fnc)                                                       \
  inline VectorRegister<type, 256> operator op(VectorRegister<type, 256> const &a,              \
                                               VectorRegister<type, 256> const &b)              \
  {                                                                                             \
    VectorRegister<type, 256>::mm_register_type imm  = _mm256_cmp_pd(a.data(), b.data(), fnc);  \
    __m256i                                     ival = _mm256_castpd_si256(imm);                \
    type                 done[VectorRegister<type, 256>::E_BLOCK_COUNT] = {1.0, 1.0, 1.0, 1.0}; \
    static const __m256i one = _mm256_castpd_si256(_mm256_load_pd(done));                       \
    __m256i              ret = _mm256_and_si256(ival, one);                                     \
    return VectorRegister<type, 256>(_mm256_castsi256_pd(ret));                                 \
  }

FETCH_ADD_OPERATOR(==, double, _CMP_EQ_OQ)
FETCH_ADD_OPERATOR(!=, double, _CMP_NEQ_OQ)
FETCH_ADD_OPERATOR(>=, double, _CMP_GE_OQ)
FETCH_ADD_OPERATOR(>, double, _CMP_GT_OQ)
FETCH_ADD_OPERATOR(<=, double, _CMP_LE_OQ)
FETCH_ADD_OPERATOR(<, double, _CMP_LT_OQ)

// Manage NaN
//__m256d _mm256_cmpord_pd (__m256d a, __m256d b)
//__m256d _mm256_cmpunord_pd (__m256d a, __m256d b)

#undef FETCH_ADD_OPERATOR

// FREE FUNCTIONS
inline VectorRegister<float, 256> max(VectorRegister<float, 256> const &a,
                                      VectorRegister<float, 256> const &b)
{
  return VectorRegister<float, 256>(_mm256_max_ps(a.data(), b.data()));
}

inline VectorRegister<float, 256> min(VectorRegister<float, 256> const &a,
                                      VectorRegister<float, 256> const &b)
{
  return VectorRegister<float, 256>(_mm256_min_ps(a.data(), b.data()));
}

inline VectorRegister<float, 256> sqrt(VectorRegister<float, 256> const &a)
{
  return VectorRegister<float, 256>(_mm256_sqrt_ps(a.data()));
}

inline VectorRegister<float, 256> approx_exp(VectorRegister<float, 256> const &x)
{
  enum
  {
    mantissa = 23,
    exponent = 8,
  };

  static constexpr float            multiplier      = float(1ull << mantissa);
  static constexpr float            exponent_offset = (float(((1ull << (exponent - 1)) - 1)));
  static VectorRegister<float, 256> a(float(multiplier / M_LN2));
  static VectorRegister<float, 256> b(float(exponent_offset * multiplier - 60801));

  VectorRegister<float, 256> y    = a * x + b;
  __m256i                    conv = _mm256_cvtps_epi32(y.data());

  return VectorRegister<float, 256>(_mm256_castsi256_ps(conv));
}

inline VectorRegister<float, 256> approx_log(VectorRegister<float, 256> const &x)
{
  enum
  {
    mantissa = 23,
    exponent = 8,
  };

  static constexpr float                  multiplier      = float(1ull << mantissa);
  static constexpr float                  exponent_offset = (float(((1ull << (exponent - 1)) - 1)));
  static const VectorRegister<float, 256> a(float(M_LN2 / multiplier));
  static const VectorRegister<float, 256> b(float(exponent_offset * multiplier - 60801));

  __m256i conv = _mm256_castps_si256(x.data());

  VectorRegister<float, 256> y(_mm256_cvtepi32_ps(conv));

  return a * (y - b);
}

inline VectorRegister<float, 256> approx_reciprocal(VectorRegister<float, 256> const &x)
{

  return VectorRegister<float, 256>(_mm256_rcp_ps(x.data()));
}

inline VectorRegister<double, 256> max(VectorRegister<double, 256> const &a,
                                       VectorRegister<double, 256> const &b)
{
  return VectorRegister<double, 256>(_mm256_max_pd(a.data(), b.data()));
}

inline VectorRegister<double, 256> min(VectorRegister<double, 256> const &a,
                                       VectorRegister<double, 256> const &b)
{
  return VectorRegister<double, 256>(_mm256_min_pd(a.data(), b.data()));
}

inline VectorRegister<double, 256> vector_zero_below_element(VectorRegister<double, 256> const &a,
                                                             int const &                        n)
{
  alignas(16) uint64_t mask[2] = {uint64_t(-(0 >= n)), uint64_t(-(1 >= n))};

  __m256i conv = _mm256_castpd_si256(a.data());
  conv         = _mm256_and_si256(conv, *(__m256i *)mask);

  return VectorRegister<double, 256>(_mm256_castsi256_pd(conv));
}

inline VectorRegister<double, 256> vector_zero_above_element(VectorRegister<double, 256> const &a,
                                                             int const &                        n)
{
  alignas(16) uint64_t mask[2] = {uint64_t(-(0 <= n)), uint64_t(-(1 <= n))};

  __m256i conv = _mm256_castpd_si256(a.data());
  conv         = _mm256_and_si256(conv, *(__m256i *)mask);

  return VectorRegister<double, 256>(_mm256_castsi256_pd(conv));
}

inline VectorRegister<float, 256> vector_zero_below_element(VectorRegister<float, 256> const &a,
                                                            int const &                       n)
{
  alignas(16) const uint32_t mask[4] = {uint32_t(-(0 >= n)), uint32_t(-(1 >= n)),
                                        uint32_t(-(2 >= n)), uint32_t(-(3 >= n))};

  __m256i conv = _mm256_castpd_si256(a.data());
  conv         = _mm256_and_si256(conv, *(__m256i *)mask);

  return VectorRegister<float, 256>(_mm256_castsi256_pd(conv));
}

inline VectorRegister<float, 256> vector_zero_above_element(VectorRegister<float, 256> const &a,
                                                            int const &                       n)
{
  alignas(16) const uint32_t mask[4] = {uint32_t(-(0 <= n)), uint32_t(-(1 <= n)),
                                        uint32_t(-(2 <= n)), uint32_t(-(3 <= n))};

  __m256i conv = _mm256_castpd_si256(a.data());
  conv         = _mm256_and_si256(conv, *(__m256i *)mask);

  return VectorRegister<float, 256>(_mm256_castsi256_pd(conv));
}

inline VectorRegister<double, 256> sqrt(VectorRegister<double, 256> const &a)
{
  return VectorRegister<double, 256>(_mm256_sqrt_pd(a.data()));
}

inline VectorRegister<float, 256> shift_elements_left(VectorRegister<float, 256> const &x)
{
  __m256 t0 = _mm256_permute_ps(x.data(), 0x39);     // [x4  x7  x6  x5  x0  x3  x2  x1]
  __m256 t1 = _mm256_permute2f128_ps(t0, t0, 0x81);  // [ 0   0   0   0  x4  x7  x6  x5]
  return VectorRegister<float, 256>(
      _mm256_blend_ps(t0, t1, 0x88));  // [ 0  x7  x6  x5  x4  x3  x2  x1]
}

inline VectorRegister<float, 256> shift_elements_right(VectorRegister<float, 256> const &x)
{
  __m256 t0 = _mm256_permute_ps(x.data(), 0x39);     // [x4  x7  x6  x5  x0  x3  x2  x1]
  __m256 t1 = _mm256_permute2f128_ps(t0, t0, 0x81);  // [ 0   0   0   0  x4  x7  x6  x5]
  return VectorRegister<float, 256>(
      _mm256_blend_ps(t0, t1, 0x88));  // [ 0  x7  x6  x5  x4  x3  x2  x1]
}

inline float first_element(VectorRegister<float, 256> const &x)
{
  return _mm256_cvtss_f32(x.data());
}

}  // namespace vectorize
}  // namespace fetch
#endif
