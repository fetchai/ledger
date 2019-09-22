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

#include "vectorise/arch/avx2/register_int32.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorise {

ADD_REGISTER_SIZE(fetch::fixed_point::fp32_t, 256);

template <>
class VectorRegister<fixed_point::fp32_t, 128> : public BaseVectorRegisterType
{
public:
  using type           = fixed_point::fp32_t;
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
  explicit VectorRegister(type const *d)
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(d->pointer()));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(list.begin()));
  }
  explicit VectorRegister(MMRegisterType const &d)
    : data_(d)
  {}
  explicit VectorRegister(MMRegisterType &&d)
    : data_(d)
  {}
  explicit VectorRegister(type const &c)
  {
    data_ = _mm_set1_epi32(c.Data());
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
class VectorRegister<fixed_point::fp32_t, 256> : public BaseVectorRegisterType
{
public:
  using type           = fixed_point::fp32_t;
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
  explicit VectorRegister(type const *d)
  {
    data_ = _mm256_load_si256(reinterpret_cast<MMRegisterType const *>(d->pointer()));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm256_load_si256(reinterpret_cast<MMRegisterType const *>(list.begin()));
  }
  explicit VectorRegister(MMRegisterType const &d)
    : data_(d)
  {}
  explicit VectorRegister(MMRegisterType &&d)
    : data_(d)
  {}
  explicit VectorRegister(type const &c)
  {
    data_ = _mm256_set1_epi32(c.Data());
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
inline std::ostream &operator<<(std::ostream &s, VectorRegister<fixed_point::fp32_t, 128> const &n)
{
  alignas(16) fixed_point::fp32_t out[4];
  n.Store(out);
  s << std::setprecision(fixed_point::fp32_t::BaseTypeInfo::decimals);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3];

  return s;
}

template <>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<fixed_point::fp32_t, 256> const &n)
{
  alignas(32) fixed_point::fp32_t out[8];
  n.Store(out);
  s << std::setprecision(fixed_point::fp32_t::BaseTypeInfo::decimals);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3] << ", " << out[4] << ", "
    << out[5] << ", " << out[6] << ", " << out[7];

  return s;
}

#define FETCH_ADD_OPERATOR(op, type, size, base_type)                                             \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &x)              \
  {                                                                                               \
    VectorRegister<base_type, size> ret = operator op(VectorRegister<base_type, size>(x.data())); \
    return VectorRegister<type, size>(ret.data());                                                \
  }

FETCH_ADD_OPERATOR(-, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(-, fixed_point::fp32_t, 256, int32_t)
#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, size, base_type)                                             \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &a,              \
                                                VectorRegister<type, size> const &b)              \
  {                                                                                               \
    VectorRegister<base_type, size> ret = operator op(VectorRegister<base_type, size>(a.data()),  \
                                                      VectorRegister<base_type, size>(b.data())); \
    return VectorRegister<type, size>(ret.data());                                                \
  }

FETCH_ADD_OPERATOR(+, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(-, fixed_point::fp32_t, 128, int32_t)

FETCH_ADD_OPERATOR(==, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(!=, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(>=, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp32_t, 128, int32_t)

FETCH_ADD_OPERATOR(+, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(-, fixed_point::fp32_t, 256, int32_t)

FETCH_ADD_OPERATOR(==, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(!=, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(>=, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp32_t, 256, int32_t)

#undef FETCH_ADD_OPERATOR

inline VectorRegister<fixed_point::fp32_t, 128> operator*(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  // Extend the vectors to 64-bit, compute the products as 64-bit
  __m256i va      = _mm256_cvtepi32_epi64(a.data());
  __m256i vb      = _mm256_cvtepi32_epi64(b.data());
  __m256i prod256 = _mm256_mul_epi32(va, vb);
  // shift the products right by 16-bits, keep only the lower 32-bits
  prod256 = _mm256_srli_epi64(prod256, 16);
  // Rearrange the 128bit lanes to keep the lower 32-bits in the first
  __m256i posmask = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
  prod256         = _mm256_permutevar8x32_epi32(prod256, posmask);
  // Extract the first 128bit lane
  VectorRegister<int32_t, 128> prod =
      VectorRegister<int32_t, 128>(_mm256_extractf128_si256(prod256, 0));
  return VectorRegister<fixed_point::fp32_t, 128>(prod.data());
}

inline VectorRegister<fixed_point::fp32_t, 256> operator*(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{
  // Use the above multiplication in 2 steps, for each 128bit lane
  VectorRegister<fixed_point::fp32_t, 128> a_lo(_mm256_extractf128_si256(a.data(), 0));
  VectorRegister<fixed_point::fp32_t, 128> a_hi(_mm256_extractf128_si256(a.data(), 1));
  VectorRegister<fixed_point::fp32_t, 128> b_lo(_mm256_extractf128_si256(b.data(), 0));
  VectorRegister<fixed_point::fp32_t, 128> b_hi(_mm256_extractf128_si256(b.data(), 1));

  VectorRegister<fixed_point::fp32_t, 128> prod_lo = a_lo * b_lo;
  VectorRegister<fixed_point::fp32_t, 128> prod_hi = a_hi * b_hi;

  VectorRegister<fixed_point::fp32_t, 256> prod(_mm256_set_m128i(prod_hi.data(), prod_lo.data()));
  return prod;
}

inline VectorRegister<fixed_point::fp32_t, 128> operator/(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  // TODO(private 440): AVX implementation required
  alignas(VectorRegister<fixed_point::fp32_t, 128>::E_REGISTER_SIZE) fixed_point::fp32_t d1[4];
  a.Store(d1);

  alignas(VectorRegister<fixed_point::fp32_t, 128>::E_REGISTER_SIZE) fixed_point::fp32_t d2[4];
  b.Store(d2);

  alignas(VectorRegister<fixed_point::fp32_t, 128>::E_REGISTER_SIZE) fixed_point::fp32_t ret[4];

  // don't divide by zero
  // set each of the 4 values in the vector register to either the solution of the division or 0
  ret[0] = d1[0] / d2[0];
  ret[1] = d1[1] / d2[1];
  ret[2] = d1[2] / d2[2];
  ret[3] = d1[3] / d2[3];

  return VectorRegister<fixed_point::fp32_t, 128>(ret);
}

inline VectorRegister<fixed_point::fp32_t, 256> operator/(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{

  // TODO(private 440): SSE implementation required
  alignas(VectorRegister<fixed_point::fp32_t, 256>::E_REGISTER_SIZE) fixed_point::fp32_t d1[8];
  a.Store(d1);

  alignas(VectorRegister<fixed_point::fp32_t, 256>::E_REGISTER_SIZE) fixed_point::fp32_t d2[8];
  b.Store(d2);

  alignas(VectorRegister<fixed_point::fp32_t, 256>::E_REGISTER_SIZE) fixed_point::fp32_t ret[8];

  // don't divide by zero
  // set each of the 4 values in the vector register to either the solution of the division or 0
  for (size_t i = 0; i < 8; i++)
  {
    ret[i] = d1[i] / d2[i];
  }

  return VectorRegister<fixed_point::fp32_t, 256>(ret);
}

inline VectorRegister<fixed_point::fp32_t, 128> vector_zero_below_element(
    VectorRegister<fixed_point::fp32_t, 128> const & /*a*/, int const & /*n*/)
{
  throw std::runtime_error("vector_zero_below_element not implemented.");
  return VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t{});
}

inline VectorRegister<fixed_point::fp32_t, 128> vector_zero_above_element(
    VectorRegister<fixed_point::fp32_t, 128> const & /*a*/, int const & /*n*/)
{
  throw std::runtime_error("vector_zero_above_element not implemented.");
  return VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t{});
}

inline VectorRegister<fixed_point::fp32_t, 128> shift_elements_left(
    VectorRegister<fixed_point::fp32_t, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_left not implemented.");
  return VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t{});
}

inline VectorRegister<fixed_point::fp32_t, 128> shift_elements_right(
    VectorRegister<fixed_point::fp32_t, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_right not implemented.");
  return VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t{});
}

inline fixed_point::fp32_t first_element(VectorRegister<fixed_point::fp32_t, 128> const &x)
{
  return fixed_point::fp32_t::FromBase(first_element(VectorRegister<int32_t, 128>(x.data())));
}

inline fixed_point::fp32_t first_element(VectorRegister<fixed_point::fp32_t, 256> const &x)
{
  return fixed_point::fp32_t::FromBase(first_element(VectorRegister<int32_t, 256>(x.data())));
}

inline fixed_point::fp32_t reduce(VectorRegister<fixed_point::fp32_t, 128> const &x)
{
  __m128i r = _mm_hadd_epi32(x.data(), _mm_setzero_si128());
  r         = _mm_hadd_epi32(r, _mm_setzero_si128());
  return fixed_point::fp32_t::FromBase(_mm_extract_epi32(r, 0));  // NOLINT
}

inline fixed_point::fp32_t reduce(VectorRegister<fixed_point::fp32_t, 256> const &x)
{
  VectorRegister<fixed_point::fp32_t, 128> hi{_mm256_extractf128_si256(x.data(), 1)};
  VectorRegister<fixed_point::fp32_t, 128> lo{_mm256_extractf128_si256(x.data(), 0)};
  hi = hi + lo;
  return reduce(hi);
}

inline bool all_less_than(VectorRegister<fixed_point::fp32_t, 128> const &x,
                          VectorRegister<fixed_point::fp32_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_less_than(VectorRegister<fixed_point::fp32_t, 256> const &x,
                          VectorRegister<fixed_point::fp32_t, 256> const &y)
{
  __m256i r = (x < y).data();
  return _mm256_movemask_epi8(r) == 0xFFFF;
}

inline bool any_less_than(VectorRegister<fixed_point::fp32_t, 128> const &x,
                          VectorRegister<fixed_point::fp32_t, 128> const &y)
{
  __m128i r = (x < y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_less_than(VectorRegister<fixed_point::fp32_t, 256> const &x,
                          VectorRegister<fixed_point::fp32_t, 256> const &y)
{
  __m256i r = (x < y).data();
  return _mm256_movemask_epi8(r) != 0;
}

inline bool all_equal_to(VectorRegister<fixed_point::fp32_t, 128> const &x,
                         VectorRegister<fixed_point::fp32_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool all_equal_to(VectorRegister<fixed_point::fp32_t, 256> const &x,
                         VectorRegister<fixed_point::fp32_t, 256> const &y)
{
  __m256i r    = (x == y).data();
  auto    mask = static_cast<uint32_t>(_mm256_movemask_epi8(r));
  return mask == 0xFFFFFFFFUL;
}

inline bool any_equal_to(VectorRegister<fixed_point::fp32_t, 128> const &x,
                         VectorRegister<fixed_point::fp32_t, 128> const &y)
{
  __m128i r = (x == y).data();
  return _mm_movemask_epi8(r) != 0;
}

inline bool any_equal_to(VectorRegister<fixed_point::fp32_t, 256> const &x,
                         VectorRegister<fixed_point::fp32_t, 256> const &y)
{
  __m256i r = (x == y).data();
  return _mm256_movemask_epi8(r) != 0;
}

}  // namespace vectorise
}  // namespace fetch
