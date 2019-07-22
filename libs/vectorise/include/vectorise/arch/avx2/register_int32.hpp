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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

#include <iostream>

namespace fetch {
namespace vectorise {

// SSE integers
template <>
class VectorRegister<int32_t, 128>
{
public:
  using type             = int32_t;
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
    data_ = _mm_load_si128((mm_register_type *)d);
  }

  VectorRegister(type const &c)
  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet<type, E_BLOCK_COUNT>::Set(constant, c);
    data_ = _mm_load_si128((mm_register_type *)constant);
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
    _mm_store_si128(reinterpret_cast<mm_register_type *>(ptr), data_);
  }

  void Stream(type *ptr) const
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

inline VectorRegister<int32_t, 128> operator-(VectorRegister<int32_t, 128> const &x)
{
  return VectorRegister<int32_t, 128>(_mm_sub_epi32(_mm_setzero_si128(), x.data()));
}

inline VectorRegister<int32_t, 128> operator+(VectorRegister<int32_t, 128> const &a,
                                              VectorRegister<int32_t, 128> const &b)
{
  __m128i ret = _mm_add_epi32(a.data(), b.data());
  return VectorRegister<int32_t, 128>(ret);
}

inline VectorRegister<int32_t, 128> operator-(VectorRegister<int32_t, 128> const &a,
                                              VectorRegister<int32_t, 128> const &b)
{
  __m128i ret = _mm_sub_epi32(a.data(), b.data());
  return VectorRegister<int32_t, 128>(ret);
}

inline VectorRegister<int32_t, 128> operator*(VectorRegister<int32_t, 128> const &a,
                                              VectorRegister<int32_t, 128> const &b)
{
  __m128i ret = _mm_mullo_epi32(a.data(), b.data());
  return VectorRegister<int32_t, 128>(ret);
}

inline VectorRegister<int32_t, 128> operator/(VectorRegister<int32_t, 128> const &a,
                                              VectorRegister<int32_t, 128> const &b)
{

  // TODO(private 440): SSE implementation required
  int32_t d1[4];
  _mm_store_si128(reinterpret_cast<__m128i *>(d1), a.data());

  int32_t d2[4];
  _mm_store_si128(reinterpret_cast<__m128i *>(d2), b.data());

  int32_t ret[4];

  // don't divide by zero
  // set each of the 4 values in the vector register to either the solution of the division or 0
  ret[0] = d2[0] != 0 ? d1[0] / d2[0] : 0;
  ret[1] = d2[1] != 0 ? d1[1] / d2[1] : 0;
  ret[2] = d2[2] != 0 ? d1[2] / d2[2] : 0;
  ret[3] = d2[3] != 0 ? d1[3] / d2[3] : 0;

  return VectorRegister<int32_t, 128>(ret);
}

inline VectorRegister<int32_t, 128> operator==(VectorRegister<int32_t, 128> const &a,
                                               VectorRegister<int32_t, 128> const &b)
{
  __m128i ret = _mm_cmpeq_epi32(a.data(), b.data());
  return VectorRegister<int32_t, 128>(ret);
}

inline VectorRegister<int32_t, 128> operator<(VectorRegister<int32_t, 128> const &a,
                                              VectorRegister<int32_t, 128> const &b)
{
  __m128i ret = _mm_cmplt_epi32(a.data(), b.data());
  return VectorRegister<int32_t, 128>(ret);
}

inline int32_t first_element(VectorRegister<int32_t, 128> const &x)
{
  return static_cast<int32_t>(_mm_extract_epi32(x.data(), 0));
}

inline VectorRegister<int32_t, 128> shift_elements_left(VectorRegister<int32_t, 128> const &x)
{
  __m128i n = _mm_bslli_si128(x.data(), 4);
  return n;
}

inline VectorRegister<int32_t, 128> shift_elements_right(VectorRegister<int32_t, 128> const &x)
{
  __m128i n = _mm_bsrli_si128(x.data(), 4);
  return n;
}

}  // namespace vectorise
}  // namespace fetch
