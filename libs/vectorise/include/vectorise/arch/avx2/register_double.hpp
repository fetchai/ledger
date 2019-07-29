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
#include <iostream>
#include <iomanip>

#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorise {

namespace details {
template <typename T, std::size_t N>
struct UnrollSet
{
  static void Set(T *ptr, T const &c)
  {
    (*ptr) = c;
    UnrollSet<T, N - 1>::Set(ptr + 1, c);
  }
};

template <typename T>
struct UnrollSet<T, 0>
{
  static void Set(T * /*ptr*/, T const & /*c*/)
  {}
};
}  // namespace details

template <>
class VectorRegister<double, 128>
{
public:
  using type             = double;
  using mm_register_type = __m128d;

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
    data_ = _mm_load_pd(d);
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm_load_pd(reinterpret_cast<type const *>(list.begin()));
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm_load_pd1(&c);
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm_store_pd(ptr, data_);
  }
  void Stream(type *ptr) const
  {
    _mm_stream_pd(ptr, data_);
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

  VectorRegister() = default;
  VectorRegister(type const *d)
  {
    data_ = _mm256_load_pd(d);
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm256_load_pd(reinterpret_cast<type const *>(list.begin()));
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm256_set1_pd(c);
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm256_store_pd(ptr, data_);
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

template<>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<double, 128> const &n)
{
  alignas(16) double out[2];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<double>::digits10);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", ";

  return s;
}

template<>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<double, 256> const &n)
{
  alignas(32) double out[4];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<double>::digits10);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3];

  return s;
}

inline VectorRegister<double, 128> operator-(VectorRegister<double, 128> const &x)
{
  return VectorRegister<double, 128>(_mm_sub_pd(_mm_setzero_pd(), x.data()));
}

inline VectorRegister<double, 256> operator-(VectorRegister<double, 256> const &x)
{
  return VectorRegister<double, 256>(_mm256_sub_pd(_mm256_setzero_pd(), x.data()));
}

#define FETCH_ADD_OPERATOR(op, type, size, L, fnc)                                       \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &a, \
                                               VectorRegister<type, size> const &b) \
  {                                                                                \
    L ret = fnc(a.data(), b.data());                                               \
    return VectorRegister<type, size>(ret);                                         \
  }

FETCH_ADD_OPERATOR(*, double, 128, __m128d, _mm_mul_pd)
FETCH_ADD_OPERATOR(-, double, 128, __m128d, _mm_sub_pd)
FETCH_ADD_OPERATOR(/, double, 128, __m128d, _mm_div_pd)
FETCH_ADD_OPERATOR(+, double, 128, __m128d, _mm_add_pd)

FETCH_ADD_OPERATOR(*, double, 256, __m256d, _mm256_mul_pd)
FETCH_ADD_OPERATOR(-, double, 256, __m256d, _mm256_sub_pd)
FETCH_ADD_OPERATOR(/, double, 256, __m256d, _mm256_div_pd)
FETCH_ADD_OPERATOR(+, double, 256, __m256d, _mm256_add_pd)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 128> operator op(VectorRegister<type, 128> const &a, \
                                               VectorRegister<type, 128> const &b) \
  {                                                                                \
    L ret  = fnc(a.data(), b.data());                                 \
    return VectorRegister<type, 128>(ret);                       \
  }

FETCH_ADD_OPERATOR(==, double, __m128d, _mm_cmpeq_pd)
FETCH_ADD_OPERATOR(!=, double, __m128d, _mm_cmpneq_pd)
FETCH_ADD_OPERATOR(>=, double, __m128d, _mm_cmpge_pd)
FETCH_ADD_OPERATOR(>, double, __m128d, _mm_cmpgt_pd)
FETCH_ADD_OPERATOR(<=, double, __m128d, _mm_cmple_pd)
FETCH_ADD_OPERATOR(<, double, __m128d, _mm_cmplt_pd)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 256> operator op(VectorRegister<type, 256> const &a, \
                                               VectorRegister<type, 256> const &b) \
  {                                                                                \
    L ret  = _mm256_cmp_pd(a.data(), b.data(), fnc);                                 \
    return VectorRegister<type, 256>(ret);                       \
  }

FETCH_ADD_OPERATOR(==, double, __m256d, _CMP_EQ_OQ)
FETCH_ADD_OPERATOR(!=, double, __m256d, _CMP_NEQ_UQ)
FETCH_ADD_OPERATOR(>=, double, __m256d, _CMP_GE_OQ)
FETCH_ADD_OPERATOR(>, double, __m256d, _CMP_GT_OQ)
FETCH_ADD_OPERATOR(<=, double, __m256d, _CMP_LE_OQ)
FETCH_ADD_OPERATOR(<, double, __m256d, _CMP_LT_OQ)

#undef FETCH_ADD_OPERATOR

// Note: Useful functions to manage NaN
//__m128d _mm_cmpord_pd (__m128d a, __m128d b)
//__m128d _mm_cmpunord_pd (__m128d a, __m128d b)

// FREE FUNCTIONS

inline VectorRegister<double, 128> vector_zero_below_element(VectorRegister<double, 128> const &a,
                                                             int const &                        n)
{
  alignas(16) uint64_t mask[2] = {uint64_t(-(0 >= n)), uint64_t(-(1 >= n))};

  __m128i conv = _mm_castpd_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i *>(mask));

  return VectorRegister<double, 128>(_mm_castsi128_pd(conv));
}

inline VectorRegister<double, 256> vector_zero_above_element(VectorRegister<double, 256> const &a,
                                                             int const &                        n)
{
  alignas(32) uint64_t mask[4] = {uint64_t(-(0 <= n)), uint64_t(-(1 <= n))};

  __m256i conv = _mm256_castpd_si256(a.data());
  conv         = _mm256_and_si256(conv, *reinterpret_cast<__m256i *>(mask));

  return VectorRegister<double, 256>(_mm256_castsi256_pd(conv));
}

inline VectorRegister<double, 128> shift_elements_left(VectorRegister<double, 128> const &x)
{
  __m128i n = _mm_castpd_si128(x.data());
  n         = _mm_bslli_si128(n, 8);
  return VectorRegister<double, 128>(_mm_castsi128_pd(n));
}

inline VectorRegister<double, 256> shift_elements_left(VectorRegister<double, 256> const &x)
{
  __m256i n = _mm256_castpd_si256(x.data());
  n         = _mm256_bslli_epi128(n, 8);
  return VectorRegister<double, 256>(_mm256_castsi256_pd(n));
}

inline VectorRegister<double, 128> shift_elements_right(VectorRegister<double, 128> const &x)
{
  __m128i n = _mm_castpd_si128(x.data());
  n         = _mm_bsrli_si128(n, 8);
  return VectorRegister<double, 128>(_mm_castsi128_pd(n));
}

inline VectorRegister<double, 256> shift_elements_right(VectorRegister<double, 256> const &x)
{
  __m256i n = _mm256_castpd_si256(x.data());
  n         = _mm256_bsrli_epi128(n, 8);
  return VectorRegister<double, 256>(_mm256_castsi256_pd(n));
}

inline double first_element(VectorRegister<double, 128> const &x)
{
  return _mm_cvtsd_f64(x.data());
}

inline double first_element(VectorRegister<double, 256> const &x)
{
  return _mm256_cvtsd_f64(x.data());
}

inline double reduce(VectorRegister<double, 128> const &x)
{
  __m128d r = _mm_hadd_pd(x.data(), _mm_setzero_pd());
  return _mm_cvtsd_f64(r);
}

inline double reduce(VectorRegister<double, 256> const &x)
{
  __m256d r = _mm256_hadd_pd(x.data(), _mm256_setzero_pd());
  return _mm256_cvtsd_f64(r);
}

inline bool all_less_than(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128((x < y).data());
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_less_than(VectorRegister<double, 256> const &x,
                          VectorRegister<double, 256> const &y)
{
  __m256i r = _mm256_castpd_si256((x < y).data());
  return _mm256_movemask_epi8(r) == 0xFFFF;
}

inline bool any_less_than(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128((x < y).data());
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_less_than(VectorRegister<double, 256> const &x,
                          VectorRegister<double, 256> const &y)
{
  __m256i r = _mm256_castpd_si256((x < y).data());
  return _mm256_movemask_epi8(r) != 0;
}

inline bool all_equal_to(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128((x == y).data());
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_equal_to(VectorRegister<double, 256> const &x,
                          VectorRegister<double, 256> const &y)
{
  __m256i r = _mm256_castpd_si256((x == y).data());
  uint32_t mask = _mm256_movemask_epi8(r);
  return mask == 0xFFFFFFFFUL;
}

inline bool any_equal_to(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128((x == y).data());
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_equal_to(VectorRegister<double, 256> const &x,
                          VectorRegister<double, 256> const &y)
{
  __m256i r = _mm256_castpd_si256((x == y).data());
  return _mm256_movemask_epi8(r) != 0;
}
}  // namespace vectorise
}  // namespace fetch
