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

namespace fetch {
namespace vectorise {

inline VectorRegister<float, 128> approx_exp(VectorRegister<float, 128> const &x)
{
  enum
  {
    mantissa = 23,
    exponent = 8,
  };

  constexpr float                  multiplier      = float(1ull << mantissa);
  constexpr float                  exponent_offset = (float(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<float, 128> a(float(multiplier / M_LN2));
  const VectorRegister<float, 128> b(float(exponent_offset * multiplier - 60801));

  VectorRegister<float, 128> y    = a * x + b;
  __m128i                    conv = _mm_cvtps_epi32(y.data());

  return VectorRegister<float, 128>(_mm_castsi128_ps(conv));
}

inline VectorRegister<float, 256> approx_exp(VectorRegister<float, 256> const &x)
{
  enum
  {
    mantissa = 23,
    exponent = 8,
  };

  constexpr float                  multiplier      = float(1ull << mantissa);
  constexpr float                  exponent_offset = (float(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<float, 256> a(float(multiplier / M_LN2));
  const VectorRegister<float, 256> b(float(exponent_offset * multiplier - 60801));

  VectorRegister<float, 256> y    = a * x + b;
  __m256i                    conv = _mm256_cvtps_epi32(y.data());

  return VectorRegister<float, 256>(_mm256_castsi256_ps(conv));
}

inline VectorRegister<double, 128> approx_exp(VectorRegister<double, 128> const &x)
{
  enum
  {
    mantissa = 20,
    exponent = 11,
  };

  alignas(16) constexpr uint64_t    mask[2]         = {uint64_t(-1), 0};
  constexpr double                  multiplier      = double(1ull << mantissa);
  constexpr double                  exponent_offset = (double(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<double, 128> a(double(multiplier / M_LN2));
  const VectorRegister<double, 128> b(double(exponent_offset * multiplier - 60801));

  VectorRegister<double, 128> y    = a * x + b;
  __m128i                     conv = _mm_cvtpd_epi32(y.data());
  conv                             = _mm_and_si128(conv, *reinterpret_cast<__m128i const *>(mask));

  conv = _mm_shuffle_epi32(conv, 3 | (0 << 2) | (3 << 4) | (1 << 6));

  return VectorRegister<double, 128>(_mm_castsi128_pd(conv));
}

inline VectorRegister<double, 256> approx_exp(VectorRegister<double, 256> const &x)
{
  enum
  {
    mantissa = 20,
    exponent = 11,
  };

  alignas(32) constexpr uint64_t    mask[4]         = {uint64_t(-1), 0, 0, 0};
  constexpr double                  multiplier      = double(1ull << mantissa);
  constexpr double                  exponent_offset = (double(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<double, 256> a(double(multiplier / M_LN2));
  const VectorRegister<double, 256> b(double(exponent_offset * multiplier - 60801));

  VectorRegister<double, 256> y    = a * x + b;
  // _mm256_cvtpd_epu64 not available until AVX512VL + AVX512DQ, need to emulated it, disabled
  __m256i                     conv = _mm256_castpd_si256(y.data()); // _mm256_cvtpd_epu64(y.data());
  conv                             = _mm256_and_si256(conv, *reinterpret_cast<__m256i const *>(mask));

  conv = _mm256_shuffle_epi32(conv, 3 | (0 << 2) | (3 << 4) | (1 << 6));

  return VectorRegister<double, 256>(_mm256_castsi256_pd(conv));
}

}  // namespace vectorise
}  // namespace fetch
