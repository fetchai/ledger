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
#include "vectorise/arch/avx2/register_int64.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

#include <iostream>

namespace fetch {
namespace vectorise {

template <>
class VectorRegister<fixed_point::fp64_t, 128>
{
public:
  using type             = fixed_point::fp64_t;
  using mm_register_type = __m128i;

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
    data_ = _mm_load_si128(reinterpret_cast<mm_register_type const *>(d->pointer()));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm_load_si128(reinterpret_cast<mm_register_type const *>(list.begin()));
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm_set1_epi64x(c.Data());
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type * ptr) const
  {
    _mm_store_si128(reinterpret_cast<mm_register_type *>(ptr), data_);
  }

  void Stream(type * ptr) const
  {
    _mm_stream_si128(reinterpret_cast<mm_register_type *>(ptr), data_);
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
class VectorRegister<fixed_point::fp64_t, 256>
{
public:
  using type             = fixed_point::fp64_t;
  using mm_register_type = __m256i;

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
    data_ = _mm256_load_si256(reinterpret_cast<mm_register_type const *>(d->pointer()));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm256_load_si256(reinterpret_cast<mm_register_type const *>(list.begin()));
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const &c)
  {
    data_ = _mm256_set1_epi64x(c.Data());
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type * ptr) const
  {
    _mm256_store_si256(reinterpret_cast<mm_register_type *>(ptr), data_);
  }

  void Stream(type * ptr) const
  {
    _mm256_stream_si256(reinterpret_cast<mm_register_type *>(ptr), data_);
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
inline std::ostream &operator<<(std::ostream &s, VectorRegister<fixed_point::fp64_t, 128> const &n)
{
  alignas(16) fixed_point::fp64_t out[4];
  n.Store(out);
  s << std::setprecision(fixed_point::fp64_t::BaseTypeInfo::decimals);
  s << std::fixed;
  s << out[0] << ", " << out[1] << ", ";

  return s;
}

template<>
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
// FETCH_ADD_OPERATOR(*, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(/, fixed_point::fp64_t, 128, int64_t)

FETCH_ADD_OPERATOR(==, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(!=, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(>=, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp64_t, 128, int64_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp64_t, 128, int64_t)

FETCH_ADD_OPERATOR(+, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(-, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(*, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(/, fixed_point::fp64_t, 256, int64_t)

FETCH_ADD_OPERATOR(==, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(!=, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(>=, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp64_t, 256, int64_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp64_t, 256, int64_t)

#undef FETCH_ADD_OPERATOR

inline VectorRegister<fixed_point::fp64_t, 128> operator*(VectorRegister<fixed_point::fp64_t, 128> const &a,
                                              VectorRegister<fixed_point::fp64_t, 128> const &b)
{
  std::cout << "a = " << a << std::endl;
  std::cout << "b = " << b << std::endl;
  VectorRegister<int64_t, 128> prod = VectorRegister<int64_t, 128>(a.data()) * VectorRegister<int64_t, 128>(b.data());
  std::cout << "prod = " << std::hex << prod << std::dec << std::endl;
  prod = _mm_srli_epi64(prod.data(), 32);
  std::cout << "prod = " << std::hex << prod << std::dec << std::endl;
  return VectorRegister<fixed_point::fp64_t, 128>(prod.data());
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

inline fixed_point::fp64_t first_element(
    VectorRegister<fixed_point::fp64_t, 128> const & /*x*/)
{
  throw std::runtime_error("first_element not implemented.");
  return fixed_point::fp64_t{};
}

inline fixed_point::fp64_t reduce(
    VectorRegister<fixed_point::fp64_t, 128> const & /*x*/)
{
  throw std::runtime_error("reduce not implemented.");
  return fixed_point::fp64_t{};
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
  __m256i r = (x == y).data();
  uint64_t mask = _mm256_movemask_epi8(r);
  return mask == 0xFFFFFFFFUL;
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
