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

#include "vectorise/info.hpp"
#include "vectorise/arch/sse/info.hpp"
#include "vectorise/register.hpp"
#include "vectorise/arch/sse/register_int32.hpp"
#include "vectorise/arch/sse/register_float.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorize {

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

inline VectorRegister<double, 128> operator-(VectorRegister<double, 128> const &x)
{
  return VectorRegister<double, 128>(_mm_sub_pd(_mm_setzero_pd(), x.data()));
}

inline VectorRegister<double, 128> operator*(VectorRegister<double, 128> const &a,
                                           VectorRegister<double, 128> const &b)
{                                                                               
  __m128d ret = _mm_mul_pd(a.data(), b.data());
  return VectorRegister<double, 128>(ret);
}

inline VectorRegister<double, 128> operator-(VectorRegister<double, 128> const &a,
                                           VectorRegister<double, 128> const &b)
{                                                                               
  __m128d ret = _mm_sub_pd(a.data(), b.data());
  return VectorRegister<double, 128>(ret);
}

inline VectorRegister<double, 128> operator/(VectorRegister<double, 128> const &a,
                                           VectorRegister<double, 128> const &b)
{                                                                               
  __m128d ret = _mm_div_pd(a.data(), b.data());
  return VectorRegister<double, 128>(ret);
}

inline VectorRegister<double, 128> operator+(VectorRegister<double, 128> const &a,
                                           VectorRegister<double, 128> const &b)
{                                                                               
  __m128d ret = _mm_add_pd(a.data(), b.data());
  return VectorRegister<double, 128>(ret);
}

inline VectorRegister<double, 128> operator ==(VectorRegister<double, 128> const &a,
                                             VectorRegister<double, 128> const &b)
{
  __m128d        imm  = _mm_cmpeq_pd(a.data(), b.data());
  __m128i        ival = _mm_castpd_si128(imm);
  constexpr double done = double(1);
  const __m128i  one  = _mm_castpd_si128(_mm_load_pd1(&done));
  __m128i        ret  = _mm_and_si128(ival, one);
  return VectorRegister<double, 128>(_mm_castsi128_pd(ret));
}

inline VectorRegister<double, 128> operator !=(VectorRegister<double, 128> const &a,
                                             VectorRegister<double, 128> const &b)
{
  __m128d        imm  = _mm_cmpneq_pd(a.data(), b.data());
  __m128i        ival = _mm_castpd_si128(imm);
  constexpr double done = double(1);
  const __m128i  one  = _mm_castpd_si128(_mm_load_pd1(&done));
  __m128i        ret  = _mm_and_si128(ival, one);
  return VectorRegister<double, 128>(_mm_castsi128_pd(ret));
}

inline VectorRegister<double, 128> operator >=(VectorRegister<double, 128> const &a,
                                             VectorRegister<double, 128> const &b)
{
  __m128d        imm  = _mm_cmpge_pd(a.data(), b.data());
  __m128i        ival = _mm_castpd_si128(imm);
  constexpr double done = double(1);
  const __m128i  one  = _mm_castpd_si128(_mm_load_pd1(&done));
  __m128i        ret  = _mm_and_si128(ival, one);
  return VectorRegister<double, 128>(_mm_castsi128_pd(ret));
}

inline VectorRegister<double, 128> operator >(VectorRegister<double, 128> const &a,
                                             VectorRegister<double, 128> const &b)
{
  __m128d        imm  = _mm_cmpgt_pd(a.data(), b.data());
  __m128i        ival = _mm_castpd_si128(imm);
  constexpr double done = double(1);
  const __m128i  one  = _mm_castpd_si128(_mm_load_pd1(&done));
  __m128i        ret  = _mm_and_si128(ival, one);
  return VectorRegister<double, 128>(_mm_castsi128_pd(ret));
}

inline VectorRegister<double, 128> operator <=(VectorRegister<double, 128> const &a,
                                             VectorRegister<double, 128> const &b)
{
  __m128d        imm  = _mm_cmple_pd(a.data(), b.data());
  __m128i        ival = _mm_castpd_si128(imm);
  constexpr double done = double(1);
  const __m128i  one  = _mm_castpd_si128(_mm_load_pd1(&done));
  __m128i        ret  = _mm_and_si128(ival, one);
  return VectorRegister<double, 128>(_mm_castsi128_pd(ret));
}

inline VectorRegister<double, 128> operator <(VectorRegister<double, 128> const &a,
                                              VectorRegister<double, 128> const &b)
{
  __m128d        imm  = _mm_cmplt_pd(a.data(), b.data());
  __m128i        ival = _mm_castpd_si128(imm);
  constexpr double done = double(1);
  const __m128i  one  = _mm_castpd_si128(_mm_load_pd1(&done));
  __m128i        ret  = _mm_and_si128(ival, one);
  return VectorRegister<double, 128>(_mm_castsi128_pd(ret));
}


// Manage NaN
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

inline VectorRegister<double, 128> vector_zero_above_element(VectorRegister<double, 128> const &a,
                                                             int const &                        n)
{
  alignas(16) uint64_t mask[2] = {uint64_t(-(0 <= n)), uint64_t(-(1 <= n))};

  __m128i conv = _mm_castpd_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i *>(mask));

  return VectorRegister<double, 128>(_mm_castsi128_pd(conv));
}

inline VectorRegister<double, 128> shift_elements_left(VectorRegister<double, 128> const &x)
{
  __m128i n = _mm_castpd_si128(x.data());
  n         = _mm_bslli_si128(n, 8);
  return VectorRegister<double, 128>(_mm_castsi128_pd(n));
}

inline VectorRegister<double, 128> shift_elements_right(VectorRegister<double, 128> const &x)
{
  __m128i n = _mm_castpd_si128(x.data());
  n         = _mm_bsrli_si128(n, 8);
  return VectorRegister<double, 128>(_mm_castsi128_pd(n));
}

inline double first_element(VectorRegister<double, 128> const &x)
{
  return _mm_cvtsd_f64(x.data());
}

// Floats


// TODO(unknown): Rename and move
inline double reduce(VectorRegister<double, 128> const &x)
{
  __m128d r = _mm_hadd_pd(x.data(), _mm_setzero_pd());
  return _mm_cvtsd_f64(r);
}


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

}  // namespace vectorize
}  // namespace fetch