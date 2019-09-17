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

#include "vectorise/arch/avx2/register_int64.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorise {

ADD_REGISTER_SIZE(fetch::fixed_point::fp64_t, 256);

template <>
class VectorRegister<fixed_point::fp64_t, 128> : public BaseVectorRegisterType
{
public:
  using type           = fixed_point::fp64_t;
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
  VectorRegister(type const *d)
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(d->pointer()));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(list.begin()));
  }
  VectorRegister(MMRegisterType const &d)
    : data_(d)
  {}
  VectorRegister(MMRegisterType &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm_set1_epi64x(c.Data());
  }

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
class VectorRegister<fixed_point::fp64_t, 256> : public BaseVectorRegisterType
{
public:
  using type           = fixed_point::fp64_t;
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
  VectorRegister(type const *d)
  {
    data_ = _mm256_load_si256(reinterpret_cast<MMRegisterType const *>(d->pointer()));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm256_load_si256(reinterpret_cast<MMRegisterType const *>(list.begin()));
  }
  VectorRegister(MMRegisterType const &d)
    : data_(d)
  {}
  VectorRegister(MMRegisterType &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm256_set1_epi64x(c.Data());
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
inline std::ostream &operator<<(std::ostream &s, VectorRegister<fixed_point::fp64_t, 128> const &n)
{
  alignas(16) fixed_point::fp64_t out[4];
  n.Store(out);
  s << std::setprecision(fixed_point::fp64_t::BaseTypeInfo::decimals);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", ";

  return s;
}

template <>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<fixed_point::fp64_t, 256> const &n)
{
  alignas(32) fixed_point::fp64_t out[8];
  n.Store(out);
  s << std::setprecision(fixed_point::fp64_t::BaseTypeInfo::decimals);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3];

  return s;
}

#define FETCH_ADD_OPERATOR(op, type, size, base_type)                                             \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &x)              \
  {                                                                                               \
    VectorRegister<base_type, size> ret = operator op(VectorRegister<base_type, size>(x.data())); \
    return VectorRegister<type, size>(ret.data());                                                \
  }

FETCH_ADD_OPERATOR(-, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(-, fixed_point::fp64_t, 256, int64_t)
#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, size, base_type)                                             \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &a,              \
                                                VectorRegister<type, size> const &b)              \
  {                                                                                               \
    VectorRegister<base_type, size> ret = operator op(VectorRegister<base_type, size>(a.data()),  \
                                                      VectorRegister<base_type, size>(b.data())); \
    return VectorRegister<type, size>(ret.data());                                                \
  }

FETCH_ADD_OPERATOR(+, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(-, fixed_point::fp64_t, 128, int64_t)

FETCH_ADD_OPERATOR(==, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(!=, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(>=, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp64_t, 128, int64_t)

FETCH_ADD_OPERATOR(+, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(-, fixed_point::fp64_t, 256, int64_t)

FETCH_ADD_OPERATOR(==, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(!=, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(>=, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp64_t, 256, int64_t)

#undef FETCH_ADD_OPERATOR

inline std::ostream &operator<<(std::ostream &s, __int128_t const &n)
{
  s << std::setprecision(32);
  s << std::fixed;
  s << std::hex;
  s << static_cast<int64_t>(n >> 64) << static_cast<int64_t>(n);

  return s;
}

inline VectorRegister<fixed_point::fp64_t, 128> operator*(
    VectorRegister<fixed_point::fp64_t, 128> const &a,
    VectorRegister<fixed_point::fp64_t, 128> const &b)
{
  alignas(32) fixed_point::fp64_t::NextType a128[2], b128[2], prod[2];
  a128[0]       = _mm_extract_epi64(a.data(), 0);
  a128[1]       = _mm_extract_epi64(a.data(), 1);
  b128[0]       = _mm_extract_epi64(b.data(), 0);
  b128[1]       = _mm_extract_epi64(b.data(), 1);
  prod[0]       = a128[0] * b128[0];
  prod[1]       = a128[1] * b128[1];
  __m256i vprod = _mm256_load_si256(reinterpret_cast<__m256i const *>(prod));
  vprod         = _mm256_bsrli_epi128(vprod, 4);
  vprod         = _mm256_permute4x64_epi64(vprod, 0x8);
  return VectorRegister<fixed_point::fp64_t, 128>(_mm256_extractf128_si256(vprod, 0));
}

inline VectorRegister<fixed_point::fp64_t, 256> operator*(
    VectorRegister<fixed_point::fp64_t, 256> const &a,
    VectorRegister<fixed_point::fp64_t, 256> const &b)
{
  // Use the above multiplication in 2 steps, for each 128bit lane
  VectorRegister<fixed_point::fp64_t, 128> a_lo(_mm256_extractf128_si256(a.data(), 0));
  VectorRegister<fixed_point::fp64_t, 128> a_hi(_mm256_extractf128_si256(a.data(), 1));
  VectorRegister<fixed_point::fp64_t, 128> b_lo(_mm256_extractf128_si256(b.data(), 0));
  VectorRegister<fixed_point::fp64_t, 128> b_hi(_mm256_extractf128_si256(b.data(), 1));

  VectorRegister<fixed_point::fp64_t, 128> prod_lo = a_lo * b_lo;
  VectorRegister<fixed_point::fp64_t, 128> prod_hi = a_hi * b_hi;

  VectorRegister<fixed_point::fp64_t, 256> prod(_mm256_set_m128i(prod_hi.data(), prod_lo.data()));
  return prod;
}

inline VectorRegister<fixed_point::fp64_t, 128> operator/(
    VectorRegister<fixed_point::fp64_t, 128> const &a,
    VectorRegister<fixed_point::fp64_t, 128> const &b)
{
  // TODO(private 440): AVX implementation required
  alignas(VectorRegister<fixed_point::fp64_t, 128>::E_REGISTER_SIZE) fixed_point::fp64_t d1[2];
  a.Store(d1);

  alignas(VectorRegister<fixed_point::fp64_t, 128>::E_REGISTER_SIZE) fixed_point::fp64_t d2[2];
  b.Store(d2);

  alignas(VectorRegister<fixed_point::fp64_t, 128>::E_REGISTER_SIZE) fixed_point::fp64_t ret[2];

  ret[0] = d1[0] / d2[0];
  ret[1] = d1[1] / d2[1];

  return VectorRegister<fixed_point::fp64_t, 128>(ret);
}

inline VectorRegister<fixed_point::fp64_t, 256> operator/(
    VectorRegister<fixed_point::fp64_t, 256> const &a,
    VectorRegister<fixed_point::fp64_t, 256> const &b)
{

  // TODO(private 440): SSE implementation required
  alignas(VectorRegister<fixed_point::fp64_t, 256>::E_REGISTER_SIZE) fixed_point::fp64_t d1[4];
  a.Store(d1);

  alignas(VectorRegister<fixed_point::fp64_t, 256>::E_REGISTER_SIZE) fixed_point::fp64_t d2[4];
  b.Store(d2);

  alignas(VectorRegister<fixed_point::fp64_t, 256>::E_REGISTER_SIZE) fixed_point::fp64_t ret[4];

  ret[0] = d1[0] / d2[0];
  ret[1] = d1[1] / d2[1];
  ret[2] = d1[2] / d2[2];
  ret[3] = d1[3] / d2[3];

  return VectorRegister<fixed_point::fp64_t, 256>(ret);
}

inline VectorRegister<fixed_point::fp64_t, 128> vector_zero_below_element(
    VectorRegister<fixed_point::fp64_t, 128> const & /*a*/, int const & /*n*/)
{
  throw std::runtime_error("vector_zero_below_element not implemented.");
  return VectorRegister<fixed_point::fp64_t, 128>(fixed_point::fp64_t{});
}

inline VectorRegister<fixed_point::fp64_t, 128> vector_zero_above_element(
    VectorRegister<fixed_point::fp64_t, 128> const & /*a*/, int const & /*n*/)
{
  throw std::runtime_error("vector_zero_above_element not implemented.");
  return VectorRegister<fixed_point::fp64_t, 128>(fixed_point::fp64_t{});
}

inline VectorRegister<fixed_point::fp64_t, 128> shift_elements_left(
    VectorRegister<fixed_point::fp64_t, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_left not implemented.");
  return VectorRegister<fixed_point::fp64_t, 128>(fixed_point::fp64_t{});
}

inline VectorRegister<fixed_point::fp64_t, 128> shift_elements_right(
    VectorRegister<fixed_point::fp64_t, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_right not implemented.");
  return VectorRegister<fixed_point::fp64_t, 128>(fixed_point::fp64_t{});
}

inline fixed_point::fp64_t first_element(VectorRegister<fixed_point::fp64_t, 128> const &x)
{
  return fixed_point::fp64_t::FromBase(first_element(VectorRegister<int64_t, 128>(x.data())));
}

inline fixed_point::fp64_t first_element(VectorRegister<fixed_point::fp64_t, 256> const &x)
{
  return fixed_point::fp64_t::FromBase(first_element(VectorRegister<int64_t, 256>(x.data())));
}

inline fixed_point::fp64_t reduce(VectorRegister<fixed_point::fp64_t, 128> const &x)
{
  int64_t ret = reduce(VectorRegister<int64_t, 128>(x.data()));
  return fixed_point::fp64_t::FromBase(ret);
}

inline fixed_point::fp64_t reduce(VectorRegister<fixed_point::fp64_t, 256> const &x)
{
  VectorRegister<fixed_point::fp64_t, 128> hi{_mm256_extractf128_si256(x.data(), 1)};
  VectorRegister<fixed_point::fp64_t, 128> lo{_mm256_extractf128_si256(x.data(), 0)};
  hi = hi + lo;
  return reduce(hi);
}

inline bool all_less_than(VectorRegister<fixed_point::fp64_t, 128> const &x,
                          VectorRegister<fixed_point::fp64_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_less_than(VectorRegister<fixed_point::fp64_t, 256> const &x,
                          VectorRegister<fixed_point::fp64_t, 256> const &y)
{
  __m256i r = (x < y).data();
  return _mm256_movemask_epi8(r) == 0xFFFF;
}

inline bool any_less_than(VectorRegister<fixed_point::fp64_t, 128> const &x,
                          VectorRegister<fixed_point::fp64_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_less_than(VectorRegister<fixed_point::fp64_t, 256> const &x,
                          VectorRegister<fixed_point::fp64_t, 256> const &y)
{
  __m256i r = (x < y).data();
  return _mm256_movemask_epi8(r) != 0;
}

inline bool all_equal_to(VectorRegister<fixed_point::fp64_t, 128> const &x,
                         VectorRegister<fixed_point::fp64_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_equal_to(VectorRegister<fixed_point::fp64_t, 256> const &x,
                         VectorRegister<fixed_point::fp64_t, 256> const &y)
{
  __m256i  r    = (x == y).data();
  uint64_t mask = static_cast<uint64_t>(_mm256_movemask_epi8(r));
  return mask == 0xffffffffffffffffULL;
}

inline bool any_equal_to(VectorRegister<fixed_point::fp64_t, 128> const &x,
                         VectorRegister<fixed_point::fp64_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_equal_to(VectorRegister<fixed_point::fp64_t, 256> const &x,
                         VectorRegister<fixed_point::fp64_t, 256> const &y)
{
  __m256i r = (x == y).data();
  return _mm256_movemask_epi8(r) != 0;
}

}  // namespace vectorise
}  // namespace fetch
