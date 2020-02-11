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

#include <emmintrin.h>
#include <limits>

#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace vectorise {

static const fixed_point::fp64_t Exp_P01(0, 0x80000000);  //  1 / 2
static const fixed_point::fp64_t Exp_P02(0, 0x1C71C71C);  //  1 / 9
static const fixed_point::fp64_t Exp_P03(0, 0x038E38E3);  //  1 / 72
static const fixed_point::fp64_t Exp_P04(0, 0x00410410);  //  1 / 1008
static const fixed_point::fp64_t Exp_P05(0, 0x00022ACD);  //  1 / 30240

static const fixed_point::fp64_t integer_mask(0xffffffff, nullptr);  //  1 / 30240

static const fixed_point::fp64_t one_over_LN2{fixed_point::fp64_t::_1 /
                                              fixed_point::fp64_t::CONST_LN2};  //  1 / CONST_LN2

static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_INTEGER_MASK(
    static_cast<fixed_point::fp32_t>(integer_mask));
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_INTEGER_MASK(
    static_cast<fixed_point::fp32_t>(integer_mask));
// TODO (ML-522) ; implement fp64 exp vectorisation
// static const VectorRegister<fixed_point::fp64_t, 128> FP64_128_INTEGER_MASK{integer_mask};
// static const VectorRegister<fixed_point::fp64_t, 256> FP64_256_INTEGER_MASK{integer_mask};

static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_CONST_LN2(
    fixed_point::fp32_t::CONST_LN2);
static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_ONE_CONST_LN2(
    static_cast<fixed_point::fp32_t>(one_over_LN2));

static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_CONST_LN2(
    fixed_point::fp32_t::CONST_LN2);
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_ONE_CONST_LN2(
    static_cast<fixed_point::fp32_t>(one_over_LN2));

static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_EXP_P01(
    static_cast<fixed_point::fp32_t>(Exp_P01));
static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_EXP_P02(
    static_cast<fixed_point::fp32_t>(Exp_P02));
static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_EXP_P03(
    static_cast<fixed_point::fp32_t>(Exp_P03));
static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_EXP_P04(
    static_cast<fixed_point::fp32_t>(Exp_P04));
static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_EXP_P05(
    static_cast<fixed_point::fp32_t>(Exp_P05));

static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_EXP_P01(
    static_cast<fixed_point::fp32_t>(Exp_P01));
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_EXP_P02(
    static_cast<fixed_point::fp32_t>(Exp_P02));
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_EXP_P03(
    static_cast<fixed_point::fp32_t>(Exp_P03));
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_EXP_P04(
    static_cast<fixed_point::fp32_t>(Exp_P04));
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_EXP_P05(
    static_cast<fixed_point::fp32_t>(Exp_P05));

static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_ONE(fixed_point::fp32_t::_1);
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_ONE(fixed_point::fp32_t::_1);

static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_MAX_EXP(
    fixed_point::fp32_t::MAX_EXP);
static const VectorRegister<fixed_point::fp32_t, 128> FP32_128_MIN_EXP(
    fixed_point::fp32_t::MIN_EXP);
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_MAX_EXP(
    fixed_point::fp32_t::MAX_EXP);
static const VectorRegister<fixed_point::fp32_t, 256> FP32_256_MIN_EXP(
    fixed_point::fp32_t::MIN_EXP);

inline VectorRegister<fixed_point::fp32_t, 128> Exp(
    VectorRegister<fixed_point::fp32_t, 128> const &x)
{
  // Now find out which of the initial elements are +inf, -inf, NaN to fill in the final result
  auto mask_pos_inf = (x == VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf());
  auto mask_neg_inf = (x == VectorRegister<fixed_point::fp32_t, 128>::MaskNegInf());
  auto mask_nan     = VectorRegister<fixed_point::fp32_t, 128>::MaskNaN(x);

  // Handle Overflows and Underflows
  // compute masks for overflow and underflow
  auto mask_overflow  = (x > FP32_128_MAX_EXP);
  auto mask_underflow = (FP32_128_MIN_EXP > x);

  VectorRegister<fixed_point::fp32_t, 128> k;
  k = multiply_unsafe(x, FP32_128_ONE_CONST_LN2);
  k = k & FP32_128_INTEGER_MASK;

  VectorRegister<fixed_point::fp32_t, 128> r = x - multiply_unsafe(k, FP32_128_CONST_LN2);

  __m128i                                  shift_indexes = _mm_srli_epi32(k.data(), 16);
  VectorRegister<fixed_point::fp32_t, 128> e1 =
      VectorRegister<fixed_point::fp32_t, 128>(_mm_sllv_epi32(FP32_128_ONE.data(), shift_indexes));

  VectorRegister<fixed_point::fp32_t, 128> r2 = multiply_unsafe(r, r);
  VectorRegister<fixed_point::fp32_t, 128> r3 = multiply_unsafe(r2, r);
  VectorRegister<fixed_point::fp32_t, 128> r4 = multiply_unsafe(r3, r);
  VectorRegister<fixed_point::fp32_t, 128> r5 = multiply_unsafe(r4, r);
  r                                           = multiply_unsafe(r, FP32_128_EXP_P01);
  r2                                          = multiply_unsafe(r2, FP32_128_EXP_P02);
  r3                                          = multiply_unsafe(r3, FP32_128_EXP_P03);
  r4                                          = multiply_unsafe(r4, FP32_128_EXP_P04);
  r5                                          = multiply_unsafe(r5, FP32_128_EXP_P05);

  VectorRegister<fixed_point::fp32_t, 128> P  = FP32_128_ONE + r + r2 + r3 + r4 + r5;
  VectorRegister<fixed_point::fp32_t, 128> Q  = FP32_128_ONE - r + r2 - r3 + r4 - r5;
  VectorRegister<fixed_point::fp32_t, 128> e2 = P / Q;
  VectorRegister<fixed_point::fp32_t, 128> e  = e1 * e2;

  // Replace +inf in the final product using the mask
  e.data() = _mm_blendv_epi8(
      e.data(), VectorRegister<fixed_point::fp32_t, 128>::MaskPosInf().data(), mask_pos_inf.data());
  // Replace -inf in the final product using the mask
  e.data() = _mm_blendv_epi8(e.data(), VectorRegister<fixed_point::fp32_t, 128>::_0().data(),
                             (mask_neg_inf | mask_underflow).data());
  // Replace NaN in the final product using the mask
  e.data() = _mm_blendv_epi8(
      e.data(), VectorRegister<fixed_point::fp32_t, 128>(fixed_point::fp32_t::NaN).data(),
      mask_nan.data());

  // Remove NaN/Inf elements from Overflow/Underflow mask
  mask_overflow.data() =
      _mm_blendv_epi8(mask_overflow.data(), _mm_setzero_si128(), (mask_nan | mask_pos_inf).data());
  mask_underflow.data() =
      _mm_blendv_epi8(mask_underflow.data(), _mm_setzero_si128(), (mask_nan | mask_neg_inf).data());
  bool is_overflow = _mm_movemask_epi8(mask_overflow.data() | mask_underflow.data()) != 0;
  bool is_infinity = any_equal_to(mask_pos_inf | mask_neg_inf,
                                  VectorRegister<fixed_point::fp32_t, 128>::MaskAllBits());
  bool is_nan = any_equal_to(mask_nan, VectorRegister<fixed_point::fp32_t, 128>::MaskAllBits());
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_INFINITY * static_cast<uint32_t>(is_infinity);
  fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_NAN * static_cast<uint32_t>(is_nan);
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_OVERFLOW * static_cast<uint32_t>(is_overflow);

  return e;
}

inline VectorRegister<fixed_point::fp32_t, 256> Exp(
    VectorRegister<fixed_point::fp32_t, 256> const &x)
{
  // Now find out which of the initial elements are +inf, -inf, NaN to fill in the final result
  auto mask_pos_inf = (x == VectorRegister<fixed_point::fp32_t, 256>::MaskPosInf());
  auto mask_neg_inf = (x == VectorRegister<fixed_point::fp32_t, 256>::MaskNegInf());
  auto mask_nan     = VectorRegister<fixed_point::fp32_t, 256>::MaskNaN(x);

  // Handle Overflows and Underflows
  // compute masks for overflow and underflow
  auto mask_overflow  = (x > FP32_256_MAX_EXP);
  auto mask_underflow = (FP32_256_MIN_EXP > x);

  VectorRegister<fixed_point::fp32_t, 256> k;
  k = multiply_unsafe(x, FP32_256_ONE_CONST_LN2);
  k = k & FP32_256_INTEGER_MASK;

  VectorRegister<fixed_point::fp32_t, 256> r = x - multiply_unsafe(k, FP32_256_CONST_LN2);

  __m256i                                  shift_indexes = _mm256_srli_epi32(k.data(), 16);
  VectorRegister<fixed_point::fp32_t, 256> e1            = VectorRegister<fixed_point::fp32_t, 256>(
      _mm256_sllv_epi32(FP32_256_ONE.data(), shift_indexes));

  VectorRegister<fixed_point::fp32_t, 256> r2 = multiply_unsafe(r, r);
  VectorRegister<fixed_point::fp32_t, 256> r3 = multiply_unsafe(r2, r);
  VectorRegister<fixed_point::fp32_t, 256> r4 = multiply_unsafe(r3, r);
  VectorRegister<fixed_point::fp32_t, 256> r5 = multiply_unsafe(r4, r);
  r                                           = multiply_unsafe(r, FP32_256_EXP_P01);
  r2                                          = multiply_unsafe(r2, FP32_256_EXP_P02);
  r3                                          = multiply_unsafe(r3, FP32_256_EXP_P03);
  r4                                          = multiply_unsafe(r4, FP32_256_EXP_P04);
  r5                                          = multiply_unsafe(r5, FP32_256_EXP_P05);

  VectorRegister<fixed_point::fp32_t, 256> P  = FP32_256_ONE + r + r2 + r3 + r4 + r5;
  VectorRegister<fixed_point::fp32_t, 256> Q  = FP32_256_ONE - r + r2 - r3 + r4 - r5;
  VectorRegister<fixed_point::fp32_t, 256> e2 = P / Q;
  VectorRegister<fixed_point::fp32_t, 256> e  = e1 * e2;

  // Replace +inf in the final product using the mask
  e.data() = _mm256_blendv_epi8(
      e.data(), VectorRegister<fixed_point::fp32_t, 256>::MaskPosInf().data(), mask_pos_inf.data());
  // Replace -inf in the final product using the mask
  e.data() = _mm256_blendv_epi8(e.data(), VectorRegister<fixed_point::fp32_t, 256>::_0().data(),
                                (mask_neg_inf | mask_underflow).data());
  // Replace NaN in the final product using the mask
  e.data() = _mm256_blendv_epi8(
      e.data(), VectorRegister<fixed_point::fp32_t, 256>(fixed_point::fp32_t::NaN).data(),
      mask_nan.data());

  // Remove NaN/Inf elements from Overflow/Underflow mask
  mask_overflow.data()  = _mm256_blendv_epi8(mask_overflow.data(), _mm256_setzero_si256(),
                                            (mask_nan | mask_pos_inf).data());
  mask_underflow.data() = _mm256_blendv_epi8(mask_underflow.data(), _mm256_setzero_si256(),
                                             (mask_nan | mask_neg_inf).data());
  bool is_overflow      = _mm256_movemask_epi8(mask_overflow.data() | mask_underflow.data()) != 0;
  bool is_infinity      = any_equal_to(mask_pos_inf | mask_neg_inf,
                                  VectorRegister<fixed_point::fp32_t, 256>::MaskAllBits());
  bool is_nan = any_equal_to(mask_nan, VectorRegister<fixed_point::fp32_t, 256>::MaskAllBits());
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_INFINITY * static_cast<uint32_t>(is_infinity);
  fixed_point::fp32_t::fp_state |= fixed_point::fp32_t::STATE_NAN * static_cast<uint32_t>(is_nan);
  fixed_point::fp32_t::fp_state |=
      fixed_point::fp32_t::STATE_OVERFLOW * static_cast<uint32_t>(is_overflow);

  return e;
}

inline VectorRegister<fixed_point::fp64_t, 128> Exp(
    VectorRegister<fixed_point::fp64_t, 128> const &x)
{
  constexpr std::size_t           size = VectorRegister<fixed_point::fp64_t, 128>::E_BLOCK_COUNT;
  alignas(16) fixed_point::fp64_t A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = fixed_point::fp64_t::Exp(val);
  }
  return {A};
}

inline VectorRegister<fixed_point::fp64_t, 256> Exp(
    VectorRegister<fixed_point::fp64_t, 256> const &x)
{
  constexpr std::size_t           size = VectorRegister<fixed_point::fp64_t, 256>::E_BLOCK_COUNT;
  alignas(32) fixed_point::fp64_t A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = fixed_point::fp64_t::Exp(val);
  }
  return {A};
}

inline VectorRegister<float, 256> Exp(VectorRegister<float, 256> const &x)
{
  constexpr std::size_t size = VectorRegister<float, 256>::E_BLOCK_COUNT;
  alignas(32) float     A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = static_cast<float>(std::exp(static_cast<double>(val)));
  }
  return {A};
}

inline VectorRegister<double, 256> Exp(VectorRegister<double, 256> const &x)
{
  constexpr std::size_t size = VectorRegister<double, 256>::E_BLOCK_COUNT;
  alignas(32) double    A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = std::exp(val);
  }
  return {A};
}

inline VectorRegister<int64_t, 256> Exp(VectorRegister<int64_t, 256> const &x)
{
  constexpr std::size_t size = VectorRegister<int64_t, 256>::E_BLOCK_COUNT;
  alignas(32) int64_t   A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = static_cast<int64_t>(std::exp(static_cast<int64_t>(val)));
  }
  return {A};
}

inline VectorRegister<int32_t, 256> Exp(VectorRegister<int32_t, 256> const &x)
{
  constexpr std::size_t size = VectorRegister<int32_t, 256>::E_BLOCK_COUNT;
  alignas(32) int32_t   A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = static_cast<int32_t>(std::exp(static_cast<int32_t>(val)));
  }
  return {A};
}

inline VectorRegister<int16_t, 256> Exp(VectorRegister<int16_t, 256> const &x)
{
  constexpr std::size_t size = VectorRegister<int16_t, 256>::E_BLOCK_COUNT;
  alignas(32) int16_t   A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = static_cast<int16_t>(std::exp(static_cast<int16_t>(val)));
  }
  return {A};
}

inline VectorRegister<int8_t, 256> Exp(VectorRegister<int8_t, 256> const &x)
{
  constexpr std::size_t size = VectorRegister<int8_t, 256>::E_BLOCK_COUNT;
  alignas(32) int8_t    A[size];
  x.Store(A);
  for (auto &val : A)
  {
    val = static_cast<int8_t>(std::exp(static_cast<int8_t>(val)));
  }
  return {A};
}

}  // namespace vectorise
}  // namespace fetch
