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

ADD_REGISTER_SIZE(int32_t, 256);

template <>
class VectorRegister<int32_t, 128> : public BaseVectorRegisterType
{
public:
  using type           = int32_t;
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
    data_ = _mm_set1_epi32(c);
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
class VectorRegister<int32_t, 256> : public BaseVectorRegisterType
{
public:
  using type           = int32_t;
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
    data_ = _mm256_set1_epi32(c);
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
inline std::ostream &operator<<(std::ostream &s, VectorRegister<int32_t, 128> const &n)
{
  alignas(16) int32_t out[4];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<int32_t>::digits10);
  s << std::fixed;
  s << std::hex << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3];

  return s;
}

template <>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<int32_t, 256> const &n)
{
  alignas(32) int32_t out[8];
  n.Store(out);
  s << std::setprecision(std::numeric_limits<int32_t>::digits10);
  s << std::fixed;
  s << std::hex << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3] << ", " << out[4]
    << ", " << out[5] << ", " << out[6] << ", " << out[7];

  return s;
}

inline VectorRegister<int32_t, 128> operator~(VectorRegister<int32_t, 128> const &x)
{
  return {_mm_xor_si128(x.data(), _mm_cmpeq_epi32(x.data(), x.data()))};
}

inline VectorRegister<int32_t, 256> operator~(VectorRegister<int32_t, 256> const &x)
{
  return {_mm256_xor_si256(x.data(), _mm256_cmpeq_epi32(x.data(), x.data()))};
}

inline VectorRegister<int32_t, 128> operator-(VectorRegister<int32_t, 128> const &x)
{
  return {_mm_sub_epi32(_mm_setzero_si128(), x.data())};
}

inline VectorRegister<int32_t, 256> operator-(VectorRegister<int32_t, 256> const &x)
{
  return {_mm256_sub_epi32(_mm256_setzero_si256(), x.data())};
}

#define FETCH_ADD_OPERATOR(op, type, size, L, fnc)                                   \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &a, \
                                                VectorRegister<type, size> const &b) \
  {                                                                                  \
    L ret = fnc(a.data(), b.data());                                                 \
    return VectorRegister<type, size>(ret);                                          \
  }

FETCH_ADD_OPERATOR(+, int32_t, 128, __m128i, _mm_add_epi32)
FETCH_ADD_OPERATOR(-, int32_t, 128, __m128i, _mm_sub_epi32)
FETCH_ADD_OPERATOR(*, int32_t, 128, __m128i, _mm_mullo_epi32)
FETCH_ADD_OPERATOR(>, int32_t, 128, __m128i, _mm_cmpgt_epi32)
FETCH_ADD_OPERATOR(&, int32_t, 128, __m128i, _mm_and_si128)
FETCH_ADD_OPERATOR(|, int32_t, 128, __m128i, _mm_or_si128)
FETCH_ADD_OPERATOR (^, int32_t, 128, __m128i, _mm_xor_si128)
FETCH_ADD_OPERATOR(==, int32_t, 128, __m128i, _mm_cmpeq_epi32)

FETCH_ADD_OPERATOR(+, int32_t, 256, __m256i, _mm256_add_epi32)
FETCH_ADD_OPERATOR(-, int32_t, 256, __m256i, _mm256_sub_epi32)
FETCH_ADD_OPERATOR(*, int32_t, 256, __m256i, _mm256_mullo_epi32)
FETCH_ADD_OPERATOR(>, int32_t, 256, __m256i, _mm256_cmpgt_epi32)
FETCH_ADD_OPERATOR(&, int32_t, 256, __m256i, _mm256_and_si256)
FETCH_ADD_OPERATOR(|, int32_t, 256, __m256i, _mm256_or_si256)
FETCH_ADD_OPERATOR (^, int32_t, 256, __m256i, _mm256_xor_si256)
FETCH_ADD_OPERATOR(==, int32_t, 256, __m256i, _mm256_cmpeq_epi32)

#undef FETCH_ADD_OPERATOR

inline VectorRegister<int32_t, 128> operator/(VectorRegister<int32_t, 128> const &a,
                                              VectorRegister<int32_t, 128> const &b)
{
  // TODO(private 440): SSE implementation required
  alignas(16) int32_t d1[4];
  _mm_store_si128(reinterpret_cast<__m128i *>(d1), a.data());

  alignas(16) int32_t d2[4];
  _mm_store_si128(reinterpret_cast<__m128i *>(d2), b.data());

  alignas(16) int32_t ret[4];

  // don't divide by zero
  // set each of the 4 values in the vector register to either the solution of the division or 0
  ret[0] = d2[0] != 0 ? d1[0] / d2[0] : 0;
  ret[1] = d2[1] != 0 ? d1[1] / d2[1] : 0;
  ret[2] = d2[2] != 0 ? d1[2] / d2[2] : 0;
  ret[3] = d2[3] != 0 ? d1[3] / d2[3] : 0;

  return {ret};
}

inline VectorRegister<int32_t, 256> operator/(VectorRegister<int32_t, 256> const &a,
                                              VectorRegister<int32_t, 256> const &b)
{
  // TODO(private 440): SSE implementation required
  alignas(32) int32_t d1[8];
  _mm256_store_si256(reinterpret_cast<__m256i *>(d1), a.data());

  alignas(32) int32_t d2[8];
  _mm256_store_si256(reinterpret_cast<__m256i *>(d2), b.data());

  alignas(32) int32_t ret[8];

  // don't divide by zero
  // set each of the 4 values in the vector register to either the solution of the division or 0
  for (std::size_t i = 0; i < 8; i++)
  {
    ret[i] = d2[i] != 0 ? d1[i] / d2[i] : 0;
  }

  return {ret};
}

inline VectorRegister<int32_t, 128> operator!=(VectorRegister<int32_t, 128> const &a,
                                               VectorRegister<int32_t, 128> const &b)
{
  return ~(a == b);
}

inline VectorRegister<int32_t, 256> operator!=(VectorRegister<int32_t, 256> const &a,
                                               VectorRegister<int32_t, 256> const &b)
{
  return ~(a == b);
}

inline VectorRegister<int32_t, 128> operator<(VectorRegister<int32_t, 128> const &a,
                                              VectorRegister<int32_t, 128> const &b)
{
  return b > a;
}

inline VectorRegister<int32_t, 256> operator<(VectorRegister<int32_t, 256> const &a,
                                              VectorRegister<int32_t, 256> const &b)
{
  return b > a;
}

inline VectorRegister<int32_t, 128> operator<=(VectorRegister<int32_t, 128> const &a,
                                               VectorRegister<int32_t, 128> const &b)
{
  return (a < b) | (a == b);
}

inline VectorRegister<int32_t, 256> operator<=(VectorRegister<int32_t, 256> const &a,
                                               VectorRegister<int32_t, 256> const &b)
{
  return (a < b) | (a == b);
}

inline VectorRegister<int32_t, 128> operator>=(VectorRegister<int32_t, 128> const &a,
                                               VectorRegister<int32_t, 128> const &b)
{
  return (a > b) | (a == b);
}

inline VectorRegister<int32_t, 256> operator>=(VectorRegister<int32_t, 256> const &a,
                                               VectorRegister<int32_t, 256> const &b)
{
  return (a > b) | (a == b);
}

inline int32_t first_element(VectorRegister<int32_t, 128> const &x)
{
  return static_cast<int32_t>(_mm_extract_epi32(x.data(), 0));  // NOLINT
}

inline int32_t first_element(VectorRegister<int32_t, 256> const &x)
{
  return static_cast<int32_t>(_mm256_extract_epi32(x.data(), 0));
}

inline VectorRegister<int32_t, 128> shift_elements_left(VectorRegister<int32_t, 128> const &x)
{
  __m128i n = _mm_bslli_si128(x.data(), 4);
  return n;
}

inline VectorRegister<int32_t, 256> shift_elements_left(VectorRegister<int32_t, 256> const &x)
{
  __m256i n = _mm256_bslli_epi128(x.data(), 4);
  return n;
}

inline VectorRegister<int32_t, 128> shift_elements_right(VectorRegister<int32_t, 128> const &x)
{
  __m128i n = _mm_bsrli_si128(x.data(), 4);
  return n;
}

inline VectorRegister<int32_t, 256> shift_elements_right(VectorRegister<int32_t, 256> const &x)
{
  __m256i n = _mm256_bsrli_epi128(x.data(), 4);
  return n;
}

inline int32_t reduce(VectorRegister<int32_t, 128> const &x)
{
  __m128i r = _mm_hadd_epi32(x.data(), _mm_setzero_si128());
  r         = _mm_hadd_epi32(r, _mm_setzero_si128());
  return static_cast<int32_t>(_mm_extract_epi32(r, 0));  // NOLINT
}

inline int32_t reduce(VectorRegister<int32_t, 256> const &x)
{
  VectorRegister<int32_t, 128> hi{_mm256_extractf128_si256(x.data(), 1)};
  VectorRegister<int32_t, 128> lo{_mm256_extractf128_si256(x.data(), 0)};
  hi = hi + lo;
  return reduce(hi);
}

inline bool all_less_than(VectorRegister<int32_t, 128> const &x,
                          VectorRegister<int32_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_less_than(VectorRegister<int32_t, 256> const &x,
                          VectorRegister<int32_t, 256> const &y)
{
  __m256i r    = (x < y).data();
  auto    mask = static_cast<uint32_t>(_mm256_movemask_epi8(r));
  return mask == 0xFFFFFFFFUL;
}

inline bool any_less_than(VectorRegister<int32_t, 128> const &x,
                          VectorRegister<int32_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_less_than(VectorRegister<int32_t, 256> const &x,
                          VectorRegister<int32_t, 256> const &y)
{
  __m256i r = (x < y).data();
  return _mm256_movemask_epi8(r) != 0;
}

inline bool all_equal_to(VectorRegister<int32_t, 128> const &x,
                         VectorRegister<int32_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_equal_to(VectorRegister<int32_t, 256> const &x,
                         VectorRegister<int32_t, 256> const &y)
{
  __m256i r    = (x == y).data();
  auto    mask = static_cast<uint32_t>(_mm256_movemask_epi8(r));
  return mask == 0xFFFFFFFFUL;
}

inline bool any_equal_to(VectorRegister<int32_t, 128> const &x,
                         VectorRegister<int32_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_equal_to(VectorRegister<int32_t, 256> const &x,
                         VectorRegister<int32_t, 256> const &y)
{
  __m256i r = (x == y).data();
  return _mm256_movemask_epi8(r) != 0;
}

}  // namespace vectorise
}  // namespace fetch
