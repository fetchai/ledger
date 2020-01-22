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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>

#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorise {

ADD_REGISTER_SIZE(int8_t, 256);

template <>
class VectorRegister<int8_t, 128> : public BaseVectorRegisterType
{
public:
  using type           = int8_t;
  using MMRegisterType = __m128i;

  enum
  {
    E_VECTOR_SIZE   = 128,
    E_REGISTER_SIZE = sizeof(MMRegisterType),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() = default;
  VectorRegister(type const *d)  // NOLINT
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(d));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(list.begin()));
  }
  VectorRegister(type const &c)  // NOLINT
  {
    data_ = _mm_set1_epi8(c);
  }

  VectorRegister(MMRegisterType const &d)  // NOLINT
    : data_(d)
  {}

  VectorRegister(MMRegisterType &&d)  // NOLINT
    : data_(d)
  {}

  explicit operator MMRegisterType()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm_store_si128(reinterpret_cast<MMRegisterType *>(ptr), data_);
  }

  void Stream(type *ptr) const
  {
    _mm_stream_si128(reinterpret_cast<MMRegisterType *>(ptr), data_);
  }

  MMRegisterType const &data() const
  {
    return data_;
  }
  MMRegisterType &data()
  {
    return data_;
  }

private:
  MMRegisterType data_;
};

template <>
class VectorRegister<int8_t, 256> : public BaseVectorRegisterType
{
public:
  using type           = int8_t;
  using MMRegisterType = __m256i;

  enum
  {
    E_VECTOR_SIZE   = 256,
    E_REGISTER_SIZE = sizeof(MMRegisterType),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() = default;
  VectorRegister(type const *d)  // NOLINT
  {
    data_ = _mm256_load_si256(reinterpret_cast<MMRegisterType const *>(d));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm256_load_si256(reinterpret_cast<MMRegisterType const *>(list.begin()));
  }
  VectorRegister(MMRegisterType const &d)  // NOLINT
    : data_(d)
  {}
  VectorRegister(MMRegisterType &&d)  // NOLINT
    : data_(d)
  {}
  VectorRegister(type const &c)  // NOLINT
  {
    data_ = _mm256_set1_epi8(c);
  }

  explicit operator MMRegisterType()
  {
    return data_;
  }

  void Store(type *ptr) const
  {
    _mm256_store_si256(reinterpret_cast<MMRegisterType *>(ptr), data_);
  }

  void Stream(type *ptr) const
  {
    _mm256_stream_si256(reinterpret_cast<MMRegisterType *>(ptr), data_);
  }

  MMRegisterType const &data() const
  {
    return data_;
  }
  MMRegisterType &data()
  {
    return data_;
  }

private:
  MMRegisterType data_;
};

template <>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<int8_t, 128> const &n)
{
  alignas(16) int8_t out[16];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<int8_t>::digits10);
  s << std::fixed;
  s << std::hex;
  for (std::size_t i = 0; i < 16; i++)
  {
    s << static_cast<int>(out[i]);
    if (i != 15)
    {
      s << ", ";
    }
  }

  return s;
}

template <>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<int8_t, 256> const &n)
{
  alignas(32) int8_t out[32];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<int8_t>::digits10);
  s << std::fixed;
  s << std::hex;
  for (std::size_t i = 0; i < 32; i++)
  {
    s << static_cast<int>(out[i]);
    if (i != 31)
    {
      s << ", ";
    }
  }

  return s;
}

inline VectorRegister<int8_t, 128> operator~(VectorRegister<int8_t, 128> const &x)
{
  return {_mm_xor_si128(x.data(), _mm_cmpeq_epi8(x.data(), x.data()))};
}

inline VectorRegister<int8_t, 256> operator~(VectorRegister<int8_t, 256> const &x)
{
  return {_mm256_xor_si256(x.data(), _mm256_cmpeq_epi8(x.data(), x.data()))};
}

inline VectorRegister<int8_t, 128> operator-(VectorRegister<int8_t, 128> const &x)
{
  return {_mm_sub_epi8(_mm_setzero_si128(), x.data())};
}

inline VectorRegister<int8_t, 256> operator-(VectorRegister<int8_t, 256> const &x)
{
  return {_mm256_sub_epi8(_mm256_setzero_si256(), x.data())};
}

inline VectorRegister<int8_t, 128> operator*(VectorRegister<int8_t, 128> const &a,
                                             VectorRegister<int8_t, 128> const &b)
{
  __m256i a16 = _mm256_cvtepi8_epi16(a.data());
  __m256i b16 = _mm256_cvtepi8_epi16(b.data());
  __m256i c16 = _mm256_mullo_epi16(a16, b16);
  return {_mm256_cvtepi16_epi8(c16)};
}

inline VectorRegister<int8_t, 256> operator*(VectorRegister<int8_t, 256> const &a,
                                             VectorRegister<int8_t, 256> const &b)
{
  auto ahi = VectorRegister<int8_t, 128>(_mm256_extractf128_si256(a.data(), 1));
  auto alo = VectorRegister<int8_t, 128>(_mm256_extractf128_si256(a.data(), 0));
  auto bhi = VectorRegister<int8_t, 128>(_mm256_extractf128_si256(b.data(), 1));
  auto blo = VectorRegister<int8_t, 128>(_mm256_extractf128_si256(b.data(), 0));
  auto chi = ahi * bhi;
  auto clo = alo * blo;

  return {_mm256_set_m128i(chi.data(), clo.data())};
}

#define FETCH_ADD_OPERATOR(op, type, size, L, fnc)                                   \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &a, \
                                                VectorRegister<type, size> const &b) \
  {                                                                                  \
    L ret = fnc(a.data(), b.data());                                                 \
    return VectorRegister<type, size>(ret);                                          \
  }

FETCH_ADD_OPERATOR(+, int8_t, 128, __m128i, _mm_add_epi8)
FETCH_ADD_OPERATOR(-, int8_t, 128, __m128i, _mm_sub_epi8)
FETCH_ADD_OPERATOR(>, int8_t, 128, __m128i, _mm_cmpgt_epi8)
FETCH_ADD_OPERATOR(&, int8_t, 128, __m128i, _mm_and_si128)
FETCH_ADD_OPERATOR(|, int8_t, 128, __m128i, _mm_or_si128)
FETCH_ADD_OPERATOR (^, int8_t, 128, __m128i, _mm_xor_si128)
FETCH_ADD_OPERATOR(==, int8_t, 128, __m128i, _mm_cmpeq_epi8)

FETCH_ADD_OPERATOR(+, int8_t, 256, __m256i, _mm256_add_epi8)
FETCH_ADD_OPERATOR(-, int8_t, 256, __m256i, _mm256_sub_epi8)
FETCH_ADD_OPERATOR(>, int8_t, 256, __m256i, _mm256_cmpgt_epi8)
FETCH_ADD_OPERATOR(&, int8_t, 256, __m256i, _mm256_and_si256)
FETCH_ADD_OPERATOR(|, int8_t, 256, __m256i, _mm256_or_si256)
FETCH_ADD_OPERATOR (^, int8_t, 256, __m256i, _mm256_xor_si256)
FETCH_ADD_OPERATOR(==, int8_t, 256, __m256i, _mm256_cmpeq_epi8)

#undef FETCH_ADD_OPERATOR

inline VectorRegister<int8_t, 128> operator/(VectorRegister<int8_t, 128> const &a,
                                             VectorRegister<int8_t, 128> const &b)
{
  // TODO(private 440): SSE implementation required
  alignas(16) int8_t d1[16];
  _mm_store_si128(reinterpret_cast<__m128i *>(d1), a.data());

  alignas(16) int8_t d2[16];
  _mm_store_si128(reinterpret_cast<__m128i *>(d2), b.data());

  alignas(16) int8_t ret[16];

  // don't divide by zero
  // set each of the 4 values in the vector register to either the solution of the division or 0
  for (std::size_t i = 0; i < 16; i++)
  {
    ret[i] = d2[i] != 0 ? static_cast<int8_t>(d1[i] / d2[i]) : 0;
  }

  return {ret};
}

inline VectorRegister<int8_t, 256> operator/(VectorRegister<int8_t, 256> const &a,
                                             VectorRegister<int8_t, 256> const &b)
{
  // TODO(private 440): SSE implementation required
  alignas(32) int8_t d1[32];
  _mm256_store_si256(reinterpret_cast<__m256i *>(d1), a.data());

  alignas(32) int8_t d2[32];
  _mm256_store_si256(reinterpret_cast<__m256i *>(d2), b.data());

  alignas(32) int8_t ret[32];

  // don't divide by zero
  // set each of the 4 values in the vector register to either the solution of the division or 0
  for (std::size_t i = 0; i < 32; i++)
  {
    ret[i] = d2[i] != 0 ? static_cast<int8_t>(d1[i] / d2[i]) : 0;
  }

  return {ret};
}

inline VectorRegister<int8_t, 128> operator!=(VectorRegister<int8_t, 128> const &a,
                                              VectorRegister<int8_t, 128> const &b)
{
  return ~(a == b);
}

inline VectorRegister<int8_t, 256> operator!=(VectorRegister<int8_t, 256> const &a,
                                              VectorRegister<int8_t, 256> const &b)
{
  return ~(a == b);
}

inline VectorRegister<int8_t, 128> operator<(VectorRegister<int8_t, 128> const &a,
                                             VectorRegister<int8_t, 128> const &b)
{
  return b > a;
}

inline VectorRegister<int8_t, 256> operator<(VectorRegister<int8_t, 256> const &a,
                                             VectorRegister<int8_t, 256> const &b)
{
  return b > a;
}

inline VectorRegister<int8_t, 128> operator<=(VectorRegister<int8_t, 128> const &a,
                                              VectorRegister<int8_t, 128> const &b)
{
  return (a < b) | (a == b);
}

inline VectorRegister<int8_t, 256> operator<=(VectorRegister<int8_t, 256> const &a,
                                              VectorRegister<int8_t, 256> const &b)
{
  return (a < b) | (a == b);
}

inline VectorRegister<int8_t, 128> operator>=(VectorRegister<int8_t, 128> const &a,
                                              VectorRegister<int8_t, 128> const &b)
{
  return (a > b) | (a == b);
}

inline VectorRegister<int8_t, 256> operator>=(VectorRegister<int8_t, 256> const &a,
                                              VectorRegister<int8_t, 256> const &b)
{
  return (a > b) | (a == b);
}

inline int8_t first_element(VectorRegister<int8_t, 128> const &x)
{
  return static_cast<int8_t>(_mm_extract_epi8(x.data(), 0));  // NOLINT
}

inline int8_t first_element(VectorRegister<int8_t, 256> const &x)
{
  return static_cast<int8_t>(_mm256_extract_epi8(x.data(), 0));
}

template <int8_t elements>
inline VectorRegister<int8_t, 128> rotate_elements_left(VectorRegister<int8_t, 128> const &x)
{
  __m128i n = x.data();
  n         = _mm_alignr_epi8(n, n, elements);
  return {n};
}

template <int8_t elements>
inline VectorRegister<int8_t, 256> rotate_elements_left(VectorRegister<int8_t, 256> const &x);

template <>
inline VectorRegister<int8_t, 256> rotate_elements_left<0>(VectorRegister<int8_t, 256> const &x)
{
  return x;
}

#define FETCH_ROTATE_ELEMENTS_LEFT(type, L)                                                          \
  template <>                                                                                        \
  inline VectorRegister<type, 256> rotate_elements_left<L>(VectorRegister<type, 256> const &x)       \
  {                                                                                                  \
    constexpr bool is_in_high128bits = L > VectorRegister<type, 256>::E_BLOCK_COUNT / 2;             \
    constexpr int  hi_id             = is_in_high128bits ? 0 : 1;                                    \
    constexpr int  lo_id             = is_in_high128bits ? 1 : 0;                                    \
    constexpr int  L1  = is_in_high128bits ? (L - VectorRegister<type, 256>::E_BLOCK_COUNT / 2) : L; \
    __m128i        hi  = _mm256_extractf128_si256(x.data(), hi_id);                                  \
    __m128i        lo  = _mm256_extractf128_si256(x.data(), lo_id);                                  \
    __m128i        hi1 = _mm_alignr_epi8(lo, hi, L1 * sizeof(type));                                 \
    __m128i        lo1 = _mm_alignr_epi8(hi, lo, L1 * sizeof(type));                                 \
    return {_mm256_set_m128i(hi1, lo1)};                                                             \
  }

FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 1)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 2)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 3)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 4)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 5)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 6)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 7)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 8)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 9)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 10)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 11)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 12)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 13)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 14)
FETCH_ROTATE_ELEMENTS_LEFT(int8_t, 15)

template <>
inline VectorRegister<int8_t, 256> rotate_elements_left<16>(VectorRegister<int8_t, 256> const &x)
{
  __m128i hi = _mm256_extractf128_si256(x.data(), 1);
  __m128i lo = _mm256_extractf128_si256(x.data(), 0);
  return {_mm256_set_m128i(lo, hi)};
}

inline VectorRegister<int8_t, 128> shift_elements_left(VectorRegister<int8_t, 128> const &x)
{
  return {_mm_bslli_si128(x.data(), 1)};
}

inline VectorRegister<int8_t, 256> shift_elements_left(VectorRegister<int8_t, 256> const &x)
{
  return {_mm256_bslli_epi128(x.data(), 1)};
}

inline VectorRegister<int8_t, 128> shift_elements_right(VectorRegister<int8_t, 128> const &x)
{
  return {_mm_bsrli_si128(x.data(), 1)};
}

inline VectorRegister<int8_t, 256> shift_elements_right(VectorRegister<int8_t, 256> const &x)
{
  return {_mm256_bsrli_epi128(x.data(), 1)};
}

inline int8_t reduce(VectorRegister<int8_t, 128> const &x)
{
  VectorRegister<int8_t, 128> r{x};

  r = r + rotate_elements_left<8>(r);
  r = r + rotate_elements_left<4>(r);
  r = r + rotate_elements_left<2>(r);
  r = r + rotate_elements_left<1>(r);
  return static_cast<int8_t>(_mm_extract_epi8(r.data(), 0));  // NOLINT
}

inline int8_t reduce(VectorRegister<int8_t, 256> const &x)
{
  VectorRegister<int8_t, 256> r{x};

  r = r + rotate_elements_left<16>(r);
  r = r + rotate_elements_left<8>(r);
  r = r + rotate_elements_left<4>(r);
  r = r + rotate_elements_left<2>(r);
  r = r + rotate_elements_left<1>(r);
  return static_cast<int8_t>(_mm256_extract_epi8(r.data(), 0));  // NOLINT
}

inline bool all_less_than(VectorRegister<int8_t, 128> const &x,
                          VectorRegister<int8_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_less_than(VectorRegister<int8_t, 256> const &x,
                          VectorRegister<int8_t, 256> const &y)
{
  __m256i r    = (x < y).data();
  auto    mask = static_cast<uint32_t>(_mm256_movemask_epi8(r));
  return mask == 0xFFFFFFFFUL;
}

inline bool any_less_than(VectorRegister<int8_t, 128> const &x,
                          VectorRegister<int8_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_less_than(VectorRegister<int8_t, 256> const &x,
                          VectorRegister<int8_t, 256> const &y)
{
  __m256i r = (x < y).data();
  return _mm256_movemask_epi8(r) != 0;
}

inline bool all_equal_to(VectorRegister<int8_t, 128> const &x, VectorRegister<int8_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_equal_to(VectorRegister<int8_t, 256> const &x, VectorRegister<int8_t, 256> const &y)
{
  __m256i r    = (x == y).data();
  auto    mask = static_cast<uint32_t>(_mm256_movemask_epi8(r));
  return mask == 0xFFFFFFFFUL;
}

inline bool any_equal_to(VectorRegister<int8_t, 128> const &x, VectorRegister<int8_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_equal_to(VectorRegister<int8_t, 256> const &x, VectorRegister<int8_t, 256> const &y)
{
  __m256i r = (x == y).data();
  return _mm256_movemask_epi8(r) != 0;
}

}  // namespace vectorise
}  // namespace fetch
