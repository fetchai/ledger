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

#include "vectorise/arch/avx2/register_int32.hpp"
#include "vectorise/arch/avx2/register_int64.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <ostream>
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
  VectorRegister(type const *d)  // NOLINT
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(d->pointer()));
  }
  VectorRegister(std::initializer_list<type> const &list)
  {
    data_ = _mm_load_si128(reinterpret_cast<MMRegisterType const *>(list.begin()));
  }
  VectorRegister(MMRegisterType const &d)  // NOLINT
    : data_(d)
  {}
  VectorRegister(MMRegisterType &&d)  // NOLINT
    : data_(d)
  {}
  VectorRegister(type const &c)  // NOLINT
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

  static const VectorRegister _0()
  {
    return VectorRegister(_mm_setzero_si128());
  }
  static const VectorRegister MaskNaN(VectorRegister const x)
  {
    return VectorRegister((VectorRegister<int32_t, E_VECTOR_SIZE>(x.data()) ==
                           VectorRegister<int32_t, E_VECTOR_SIZE>(fixed_point::fp32_t::NaN.Data()))
                              .data());
  }
  static const VectorRegister MaskPosInf()
  {
    return VectorRegister(fixed_point::fp32_t::POSITIVE_INFINITY);
  }
  static const VectorRegister MaskNegInf()
  {
    return VectorRegister(fixed_point::fp32_t::NEGATIVE_INFINITY);
  }
  static const VectorRegister MaskAllBits()
  {
    return VectorRegister(_mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
  }
  static const VectorRegister MaskMax()
  {
    return VectorRegister(fixed_point::fp32_t::FP_MAX);
  }
  static const VectorRegister MaskMin()
  {
    return VectorRegister(fixed_point::fp32_t::FP_MIN);
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
  VectorRegister(type const *d)  // NOLINT
  {
    data_ = _mm256_load_si256(reinterpret_cast<MMRegisterType const *>(d->pointer()));
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

  static const VectorRegister _0()
  {
    return VectorRegister(_mm256_setzero_si256());
  }
  static const VectorRegister MaskNaN(VectorRegister const x)
  {
    return VectorRegister((VectorRegister<int32_t, E_VECTOR_SIZE>(x.data()) ==
                           VectorRegister<int32_t, E_VECTOR_SIZE>(fixed_point::fp32_t::NaN.Data()))
                              .data());
  }
  static const VectorRegister MaskPosInf()
  {
    return VectorRegister(fixed_point::fp32_t::POSITIVE_INFINITY);
  }
  static const VectorRegister MaskNegInf()
  {
    return VectorRegister(fixed_point::fp32_t::NEGATIVE_INFINITY);
  }
  static const VectorRegister MaskAllBits()
  {
    return VectorRegister(_mm256_cmpeq_epi32(_mm256_setzero_si256(), _mm256_setzero_si256()));
  }
  static const VectorRegister MaskMax()
  {
    return VectorRegister(fixed_point::fp32_t::FP_MAX);
  }
  static const VectorRegister MaskMin()
  {
    return VectorRegister(fixed_point::fp32_t::FP_MIN);
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

inline VectorRegister<fixed_point::fp32_t, 128> operator~(
    VectorRegister<fixed_point::fp32_t, 128> const &x)
{
  VectorRegister<int32_t, 128> ret = operator~(VectorRegister<int32_t, 128>(x.data()));
  return {ret.data()};
}

inline VectorRegister<fixed_point::fp32_t, 256> operator~(
    VectorRegister<fixed_point::fp32_t, 256> const &x)
{
  VectorRegister<int32_t, 256> ret = operator~(VectorRegister<int32_t, 256>(x.data()));
  return {ret.data()};
}

#define FETCH_ADD_OPERATOR(op, type, size, base_type)                                             \
  inline VectorRegister<type, size> operator op(VectorRegister<type, size> const &a,              \
                                                VectorRegister<type, size> const &b)              \
  {                                                                                               \
    VectorRegister<base_type, size> ret = operator op(VectorRegister<base_type, size>(a.data()),  \
                                                      VectorRegister<base_type, size>(b.data())); \
    return {ret.data()};                                                                          \
  }

FETCH_ADD_OPERATOR(>=, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp32_t, 128, int32_t)

FETCH_ADD_OPERATOR(>=, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(>, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(<=, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(<, fixed_point::fp32_t, 256, int32_t)

FETCH_ADD_OPERATOR(&, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR(|, fixed_point::fp32_t, 128, int32_t)
FETCH_ADD_OPERATOR (^, fixed_point::fp32_t, 128, int32_t)

FETCH_ADD_OPERATOR(&, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR(|, fixed_point::fp32_t, 256, int32_t)
FETCH_ADD_OPERATOR (^, fixed_point::fp32_t, 256, int32_t)

#undef FETCH_ADD_OPERATOR

inline VectorRegister<fixed_point::fp32_t, 128> operator==(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  auto                         mask_nan_a = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(a);
  auto                         mask_nan_b = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(b);
  VectorRegister<int32_t, 128> ret =
      (VectorRegister<int32_t, 128>(a.data()) == VectorRegister<int32_t, 128>(b.data()));
  ret.data() = _mm_blendv_epi8(ret.data(), _mm_setzero_si128(), (mask_nan_a | mask_nan_b).data());
  return {ret.data()};
}

inline VectorRegister<fixed_point::fp32_t, 256> operator==(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{
  auto                         mask_nan_a = VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(a);
  auto                         mask_nan_b = VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(b);
  VectorRegister<int32_t, 256> ret =
      (VectorRegister<int32_t, 256>(a.data()) == VectorRegister<int32_t, 256>(b.data()));
  ret.data() =
      _mm256_blendv_epi8(ret.data(), _mm256_setzero_si256(), (mask_nan_a | mask_nan_b).data());
  return {ret.data()};
}

inline VectorRegister<fixed_point::fp32_t, 128> operator!=(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  auto                         mask_nan_a = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(a);
  auto                         mask_nan_b = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(b);
  VectorRegister<int32_t, 128> ret =
      (VectorRegister<int32_t, 128>(a.data()) != VectorRegister<int32_t, 128>(b.data()));
  ret.data() = _mm_blendv_epi8(ret.data(), _mm_setzero_si128(), (mask_nan_a | mask_nan_b).data());
  return {ret.data()};
}

inline VectorRegister<fixed_point::fp32_t, 256> operator!=(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{
  auto                         mask_nan_a = VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(a);
  auto                         mask_nan_b = VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(b);
  VectorRegister<int32_t, 256> ret =
      (VectorRegister<int32_t, 256>(a.data()) != VectorRegister<int32_t, 256>(b.data()));
  ret.data() =
      _mm256_blendv_epi8(ret.data(), _mm256_setzero_si256(), (mask_nan_a | mask_nan_b).data());
  return {ret.data()};
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
  __m256i r    = (x < y).data();
  auto    mask = static_cast<uint32_t>(_mm256_movemask_epi8(r));
  return mask == 0xFFFFFFFFUL;
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

inline VectorRegister<fixed_point::fp32_t, 128> operator-(
    VectorRegister<fixed_point::fp32_t, 128> const &x)
{
  VectorRegister<fixed_point::fp32_t, 128> mask =
      VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(x);
  VectorRegister<int32_t, 128> ret = operator-(VectorRegister<int32_t, 128>(x.data()));
  ret.data()                       = _mm_blendv_epi8(
      ret.data(), VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t::NaN).data(),
      mask.data());
  return {ret.data()};
}

inline VectorRegister<fixed_point::fp32_t, 256> operator-(
    VectorRegister<fixed_point::fp32_t, 256> const &x)
{
  VectorRegister<fixed_point::fp32_t, 256> mask =
      VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(x);
  VectorRegister<int32_t, 256> ret = operator-(VectorRegister<int32_t, 256>(x.data()));
  ret.data()                       = _mm256_blendv_epi8(
      ret.data(), VectorRegister<fixed_point::fp32_t, 256>(fixed_point::fp32_t::NaN).data(),
      mask.data());
  return {ret.data()};
}

inline VectorRegister<fixed_point::fp32_t, 128> operator+(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  // Compute sum of vectors
  VectorRegister<fixed_point::fp32_t, 128> sum(_mm_add_epi32(a.data(), b.data()));

  // Following Agner Fog's tip for overflows/underflows as found in
  // https://www.agner.org/optimize/nan_propagation.pdf if (b > 0 && a > FP_MAX -b)  -> a + b will
  // overflow if (b < 0 && a < FP_MIN -b)  -> a + b will underflow Find positive and negative b
  // elements
  __m128i b_pos = _mm_cmpgt_epi32(b.data(), _mm_setzero_si128());
  __m128i b_neg = _mm_cmplt_epi32(b.data(), _mm_setzero_si128());

  // Handle Overflows/Underflows
  // compute FP_MAX - b and FP_MIN -b
  __m128i max            = _mm_set1_epi32(fixed_point::fp32_t::MAX);
  __m128i min            = _mm_set1_epi32(fixed_point::fp32_t::MIN);
  __m128i max_b          = _mm_sub_epi32(max, b.data());
  __m128i min_b          = _mm_sub_epi32(min, b.data());
  __m128i mask_overflow  = _mm_cmpgt_epi32(a.data(), max_b);
  __m128i mask_underflow = _mm_cmplt_epi32(a.data(), min_b);

  // Compute the overflow/underflow masks and respectively set values to MAX/MIN where appropriate
  mask_overflow  = _mm_and_si128(mask_overflow, b_pos);
  mask_underflow = _mm_and_si128(mask_underflow, b_neg);
  sum.data()     = _mm_blendv_epi8(sum.data(), max, mask_overflow);
  sum.data()     = _mm_blendv_epi8(sum.data(), min, mask_underflow);

  // Now find out which of the initial elements are +inf, -inf, NaN to fill in the final result
  auto mask_pos_inf_a = (a == VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf());
  auto mask_pos_inf_b = (b == VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf());
  auto mask_neg_inf_a = (a == VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf());
  auto mask_neg_inf_b = (b == VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf());
  auto mask_nan_a     = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(a);
  auto mask_nan_b     = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(b);

  // Compute the masks for +/-infinity
  // +inf * anything other than 0, nan, -inf is +inf, -inf * -inf = +inf
  auto mask_pos_inf = mask_pos_inf_a | mask_pos_inf_b;
  auto mask_neg_inf = mask_neg_inf_a | mask_neg_inf_b;

  // Handle NaN, produce the resp. mask and modifu infinity masks
  // +inf * 0 = NaN, -inf * 0 = NaN, 0 * +inf = NaN, 0 * -inf = NaN
  auto mask_nan = (mask_pos_inf_a & mask_neg_inf_b) | (mask_neg_inf_a & mask_pos_inf_b) |
                  (mask_nan_a | mask_nan_b);
  mask_pos_inf.data() = _mm_blendv_epi8(mask_pos_inf.data(), _mm_setzero_si128(), mask_nan.data());
  mask_neg_inf.data() = _mm_blendv_epi8(mask_neg_inf.data(), _mm_setzero_si128(), mask_nan.data());

  // Replace +inf in the final product using the mask
  sum.data() =
      _mm_blendv_epi8(sum.data(), VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf().data(),
                      mask_pos_inf.data());
  // Replace -inf in the final product using the mask
  sum.data() =
      _mm_blendv_epi8(sum.data(), VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf().data(),
                      mask_neg_inf.data());
  // Replace NaN in the final product using the mask
  sum.data() = _mm_blendv_epi8(
      sum.data(), VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t::NaN).data(),
      mask_nan.data());

  // Remove NaN/Inf elements from Overflow/Underflow mask
  mask_overflow =
      _mm_blendv_epi8(mask_overflow, _mm_setzero_si128(), (mask_nan | mask_pos_inf).data());
  mask_underflow =
      _mm_blendv_epi8(mask_underflow, _mm_setzero_si128(), (mask_nan | mask_neg_inf).data());
  bool is_overflow = _mm_movemask_epi8(mask_overflow | mask_underflow) != 0;
  bool is_infinity = any_equal_to(mask_pos_inf | mask_neg_inf,
                                  VectorRegister<fixed_point::fp32_t, 128>::MaskAllBits());
  bool is_nan = any_equal_to(mask_nan, VectorRegister<fixed_point::fp32_t, 128>::MaskAllBits());
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_INFINITY * static_cast<uint32_t>(is_infinity);
  fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_NAN * static_cast<uint32_t>(is_nan);
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_OVERFLOW * static_cast<uint32_t>(is_overflow);

  return sum;
}

inline VectorRegister<fixed_point::fp32_t, 256> operator+(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{
  // Compute sum of vectors
  VectorRegister<fixed_point::fp32_t, 256> sum(_mm256_add_epi32(a.data(), b.data()));

  // Following Agner Fog's tip for overflows/underflows as found in
  // https://www.agner.org/optimize/nan_propagation.pdf if (b > 0 && a > FP_MAX -b)  -> a + b will
  // overflow if (b < 0 && a < FP_MIN -b)  -> a + b will underflow Find positive and negative b
  // elements
  __m256i b_pos = _mm256_cmpgt_epi32(b.data(), _mm256_setzero_si256());
  __m256i b_neg = _mm256_cmpgt_epi32(_mm256_setzero_si256(), b.data());

  // Handle Overflows/Underflows
  // compute FP_MAX - b and FP_MIN -b
  __m256i max            = _mm256_set1_epi32(fixed_point::fp32_t::MAX);
  __m256i min            = _mm256_set1_epi32(fixed_point::fp32_t::MIN);
  __m256i max_b          = _mm256_sub_epi32(max, b.data());
  __m256i min_b          = _mm256_sub_epi32(min, b.data());
  __m256i mask_overflow  = _mm256_cmpgt_epi32(a.data(), max_b);
  __m256i mask_underflow = _mm256_cmpgt_epi32(min_b, a.data());

  // Compute the overflow/underflow masks and respectively set values to MAX/MIN where appropriate
  mask_overflow  = _mm256_and_si256(mask_overflow, b_pos);
  mask_underflow = _mm256_and_si256(mask_underflow, b_neg);
  sum.data()     = _mm256_blendv_epi8(sum.data(), max, mask_overflow);
  sum.data()     = _mm256_blendv_epi8(sum.data(), min, mask_underflow);

  // Now find out which of the initial elements are +inf, -inf, NaN to fill in the final result
  auto mask_pos_inf_a = (a == VectorRegister<fixed_point::fp32_t, 256>::MaskPosInf());
  auto mask_pos_inf_b = (b == VectorRegister<fixed_point::fp32_t, 256>::MaskPosInf());
  auto mask_neg_inf_a = (a == VectorRegister<fixed_point::fp32_t, 256>::MaskNegInf());
  auto mask_neg_inf_b = (b == VectorRegister<fixed_point::fp32_t, 256>::MaskNegInf());
  auto mask_nan_a     = VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(a);
  auto mask_nan_b     = VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(b);

  // Compute the masks for +/-infinity
  // +inf * anything other than 0, nan, -inf is +inf, -inf * -inf = +inf
  auto mask_pos_inf = mask_pos_inf_a | mask_pos_inf_b;
  auto mask_neg_inf = mask_neg_inf_a | mask_neg_inf_b;

  // Handle NaN, produce the resp. mask and modifu infinity masks
  // +inf * 0 = NaN, -inf * 0 = NaN, 0 * +inf = NaN, 0 * -inf = NaN
  auto mask_nan = (mask_pos_inf_a & mask_neg_inf_b) | (mask_neg_inf_a & mask_pos_inf_b) |
                  (mask_nan_a | mask_nan_b);
  mask_pos_inf.data() =
      _mm256_blendv_epi8(mask_pos_inf.data(), _mm256_setzero_si256(), mask_nan.data());
  mask_neg_inf.data() =
      _mm256_blendv_epi8(mask_neg_inf.data(), _mm256_setzero_si256(), mask_nan.data());

  // Replace +inf in the final product using the mask
  sum.data() =
      _mm256_blendv_epi8(sum.data(), VectorRegister<fixed_point::fp32_t, 256>::MaskPosInf().data(),
                         mask_pos_inf.data());
  // Replace -inf in the final product using the mask
  sum.data() =
      _mm256_blendv_epi8(sum.data(), VectorRegister<fixed_point::fp32_t, 256>::MaskNegInf().data(),
                         mask_neg_inf.data());
  // Replace NaN in the final product using the mask
  sum.data() = _mm256_blendv_epi8(
      sum.data(), VectorRegister<fixed_point::fp32_t, 256>(fixed_point::fp32_t::NaN).data(),
      mask_nan.data());

  // Remove NaN/Inf elements from Overflow/Underflow mask
  mask_overflow =
      _mm256_blendv_epi8(mask_overflow, _mm256_setzero_si256(), (mask_nan | mask_pos_inf).data());
  mask_underflow =
      _mm256_blendv_epi8(mask_underflow, _mm256_setzero_si256(), (mask_nan | mask_neg_inf).data());
  bool is_overflow = _mm256_movemask_epi8(mask_overflow | mask_underflow) != 0;
  bool is_infinity = any_equal_to(mask_pos_inf | mask_neg_inf,
                                  VectorRegister<fixed_point::fp32_t, 256>::MaskAllBits());
  bool is_nan = any_equal_to(mask_nan, VectorRegister<fixed_point::fp32_t, 256>::MaskAllBits());
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_INFINITY * static_cast<uint32_t>(is_infinity);
  fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_NAN * static_cast<uint32_t>(is_nan);
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_OVERFLOW * static_cast<uint32_t>(is_overflow);

  return sum;
}

inline VectorRegister<fixed_point::fp32_t, 128> operator-(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  return a + (-b);
}

inline VectorRegister<fixed_point::fp32_t, 256> operator-(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{
  return a + (-b);
}

inline VectorRegister<fixed_point::fp32_t, 128> operator*(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  // Extend the vectors to 64-bit, compute the products as 64-bit
  __m256i va      = _mm256_cvtepi32_epi64(a.data());
  __m256i vb      = _mm256_cvtepi32_epi64(b.data());
  __m256i prod256 = _mm256_mul_epi32(va, vb);

  // compute mask of elements larger than FP_MAX and smaller than FP_MIN
  __m256i max      = _mm256_set1_epi64x(fixed_point::fp32_t::MAX);
  __m256i min      = _mm256_set1_epi64x(fixed_point::fp32_t::MIN);
  __m256i mask_max = _mm256_cmpgt_epi64(prod256, max);
  __m256i mask_min = _mm256_cmpgt_epi64(min, prod256);

  // shift the products right by 16-bits, keep only the lower 32-bits
  prod256 = _mm256_srli_epi64(prod256, 16);
  prod256 = _mm256_blendv_epi8(prod256, max, mask_max);
  prod256 = _mm256_blendv_epi8(prod256, min, mask_min);

  // Rearrange the 128bit lanes to keep the lower 32-bits in the first
  __m256i posmask = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
  prod256         = _mm256_permutevar8x32_epi32(prod256, posmask);
  // Extract the first 128bit lane
  auto prod           = VectorRegister<int32_t, 128>(_mm256_extractf128_si256(prod256, 0));
  auto mask_overflow  = _mm256_extractf128_si256(_mm256_permutevar8x32_epi32(mask_max, posmask), 0);
  auto mask_underflow = _mm256_extractf128_si256(_mm256_permutevar8x32_epi32(mask_min, posmask), 0);

  // Find the negative elements of a, b, used to change the sign in infinities
  auto a_neg = a < VectorRegister<fixed_point::fp32_t, 128>(_mm_setzero_si128());
  auto b_neg = b < VectorRegister<fixed_point::fp32_t, 128>(_mm_setzero_si128());

  // Now find out which of the initial elements are +inf, -inf, NaN to fill in the final result
  auto mask_pos_inf_a = (a == VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf());
  auto mask_pos_inf_b = (b == VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf());
  auto mask_neg_inf_a = (a == VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf());
  auto mask_neg_inf_b = (b == VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf());
  auto mask_nan_a     = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(a);
  auto mask_nan_b     = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(b);

  // Compute the masks for +/-infinity
  // +inf * anything other than 0, nan, -inf is +/-inf, -inf * -inf = +inf, a * -inf = +inf when a <
  // 0, -inf * b = +inf when b < 0
  auto mask_neg_inf = (mask_neg_inf_a & mask_pos_inf_b) | (mask_pos_inf_a & mask_neg_inf_b) |
                      (mask_neg_inf_a & ~b_neg) | (~a_neg & mask_neg_inf_b) |
                      (mask_pos_inf_a & b_neg) | (a_neg & mask_pos_inf_b);
  auto mask_pos_inf = (mask_pos_inf_a | mask_pos_inf_b) | (mask_neg_inf_a & mask_neg_inf_b) |
                      (a_neg & mask_neg_inf_b) | (mask_neg_inf_a & b_neg);
  auto mask_zero_a = (a == VectorRegister<fixed_point::fp32_t, 128>(_mm_setzero_si128()));
  auto mask_zero_b = (b == VectorRegister<fixed_point::fp32_t, 128>(_mm_setzero_si128()));

  // +inf * 0 = NaN, -inf * 0 = NaN, 0 * +inf = NaN, 0 * -inf = NaN
  auto mask_nan = (mask_pos_inf_a & mask_zero_b) | (mask_neg_inf_a & mask_zero_b) |
                  (mask_zero_a & mask_pos_inf_b) | (mask_zero_a & mask_neg_inf_b) |
                  (mask_nan_a | mask_nan_b);
  mask_pos_inf.data() = _mm_blendv_epi8(mask_pos_inf.data(), _mm_setzero_si128(), mask_nan.data());
  mask_neg_inf.data() = _mm_blendv_epi8(mask_neg_inf.data(), _mm_setzero_si128(), mask_nan.data());

  mask_overflow =
      _mm_blendv_epi8(mask_overflow, _mm_setzero_si128(), (mask_nan | mask_pos_inf).data());
  mask_underflow =
      _mm_blendv_epi8(mask_underflow, _mm_setzero_si128(), (mask_nan | mask_neg_inf).data());
  // Replace +inf in the final product using the mask
  prod.data() =
      _mm_blendv_epi8(prod.data(), VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf().data(),
                      mask_pos_inf.data());
  // Replace -inf in the final product using the mask
  prod.data() =
      _mm_blendv_epi8(prod.data(), VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf().data(),
                      mask_neg_inf.data());
  // Replace NaN in the final product using the mask
  prod.data() = _mm_blendv_epi8(
      prod.data(), VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t::NaN).data(),
      mask_nan.data());

  bool is_overflow = _mm_movemask_epi8(mask_overflow | mask_underflow) != 0;
  bool is_infinity = any_equal_to(mask_pos_inf | mask_neg_inf,
                                  VectorRegister<fixed_point::fp32_t, 128>::MaskAllBits());
  bool is_nan = any_equal_to(mask_nan, VectorRegister<fixed_point::fp32_t, 128>::MaskAllBits());
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_INFINITY * static_cast<uint32_t>(is_infinity);
  fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_NAN * static_cast<uint32_t>(is_nan);
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_OVERFLOW * static_cast<uint32_t>(is_overflow);

  return {prod.data()};
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

  return {ret};
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
  for (std::size_t i = 0; i < 8; i++)
  {
    ret[i] = d1[i] / d2[i];
  }

  return {ret};
}

inline VectorRegister<fixed_point::fp32_t, 128> vector_zero_below_element(
    VectorRegister<fixed_point::fp32_t, 128> const & /*a*/, int const & /*n*/)
{
  throw std::runtime_error("vector_zero_below_element not implemented.");
  return {fixed_point::fp32_t{}};
}

inline VectorRegister<fixed_point::fp32_t, 128> vector_zero_above_element(
    VectorRegister<fixed_point::fp32_t, 128> const & /*a*/, int const & /*n*/)
{
  throw std::runtime_error("vector_zero_above_element not implemented.");
  return {fixed_point::fp32_t{}};
}

inline VectorRegister<fixed_point::fp32_t, 128> shift_elements_left(
    VectorRegister<fixed_point::fp32_t, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_left not implemented.");
  return {fixed_point::fp32_t{}};
}

inline VectorRegister<fixed_point::fp32_t, 128> shift_elements_right(
    VectorRegister<fixed_point::fp32_t, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_right not implemented.");
  return {fixed_point::fp32_t{}};
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
  bool is_pos_inf = any_equal_to(x.data(), VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf());
  bool is_neg_inf = any_equal_to(x.data(), VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf());
  bool is_nan     = any_equal_to(VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(x),
                             VectorRegister<fixed_point::fp32_t, 128>::MaskAllBits());

  is_nan &= is_pos_inf && is_neg_inf;
  if (is_nan)
  {
    fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_NAN;
    return fixed_point::fp32_t::NaN;
  }
  if (is_pos_inf)
  {
    fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_INFINITY;
    return fixed_point::fp32_t::POSITIVE_INFINITY;
  }
  if (is_neg_inf)
  {
    fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_INFINITY;
    return fixed_point::fp32_t::NEGATIVE_INFINITY;
  }
  // We can't use AVX _mm_hadd_epi32 in this case as we need to check for overflows, but we still
  // can save some cycles from doing only the overflow checks vs doing the full addition per
  // element

  alignas(VectorRegister<fixed_point::fp32_t, 128>::E_REGISTER_SIZE) fixed_point::fp32_t x_[4];
  x.Store(x_);
  fixed_point::fp32_t sum{x_[0]};
  for (size_t i = 1; i < 4; i++)
  {
    if (fixed_point::fp32_t::CheckOverflow(
            static_cast<fixed_point::fp32_t::NextType>(sum.Data()) +
            static_cast<fixed_point::fp32_t::NextType>(x_[i].Data())))
    {
      fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_OVERFLOW;
      return fixed_point::fp32_t::FP_MAX;
    }
    if (fixed_point::fp32_t::CheckUnderflow(
            static_cast<fixed_point::fp32_t::NextType>(sum.Data()) +
            static_cast<fixed_point::fp32_t::NextType>(x_[i].Data())))
    {
      fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_OVERFLOW;
      return fixed_point::fp32_t::FP_MIN;
    }
    sum.Data() += x_[i].Data();
  }
  return sum;
}

inline fixed_point::fp32_t reduce(VectorRegister<fixed_point::fp32_t, 256> const &x)
{
  VectorRegister<fixed_point::fp32_t, 128> hi{_mm256_extractf128_si256(x.data(), 1)};
  VectorRegister<fixed_point::fp32_t, 128> lo{_mm256_extractf128_si256(x.data(), 0)};
  hi = hi + lo;
  return reduce(hi);
}

}  // namespace vectorise
}  // namespace fetch
