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

namespace fetch {
namespace vectorise {

inline VectorRegister<float, 128> approx_log(VectorRegister<float, 128> const &x)
{
  enum
  {
    mantissa = 23,
    exponent = 8,
  };

  constexpr auto                   multiplier      = float(1ull << mantissa);
  constexpr float                  exponent_offset = (float(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<float, 128> a(float(M_LN2 / multiplier));
  const VectorRegister<float, 128> b(float(exponent_offset * multiplier - 60801));

  __m128i conv = _mm_castps_si128(x.data());

  VectorRegister<float, 128> y(_mm_cvtepi32_ps(conv));

  return a * (y - b);
}

inline VectorRegister<float, 256> approx_log(VectorRegister<float, 256> const &x)
{
  enum
  {
    mantissa = 23,
    exponent = 8,
  };

  constexpr auto                   multiplier      = float(1ull << mantissa);
  constexpr float                  exponent_offset = (float(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<float, 256> a(float(M_LN2 / multiplier));
  const VectorRegister<float, 256> b(float(exponent_offset * multiplier - 60801));

  __m256i conv = _mm256_castps_si256(x.data());

  VectorRegister<float, 256> y(_mm256_cvtepi32_ps(conv));

  return a * (y - b);
}

inline VectorRegister<double, 128> approx_log(VectorRegister<double, 128> const &x)
{
  enum
  {
    mantissa = 20,
    exponent = 11,
  };

  alignas(16) constexpr uint64_t    mask[2]         = {uint64_t(-1), 0};
  constexpr auto                    multiplier      = double(1ull << mantissa);
  constexpr double                  exponent_offset = (double(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<double, 128> a(double(M_LN2 / multiplier));
  const VectorRegister<double, 128> b(double(exponent_offset * multiplier - 60801));

  __m128i conv = _mm_castpd_si128(x.data());
  conv         = _mm_shuffle_epi32(conv, 1 | (3 << 2) | (0 << 4) | (2 << 6));
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i const *>(mask));

  VectorRegister<double, 128> y(_mm_cvtepi32_pd(conv));

  return a * (y - b);
}

inline VectorRegister<double, 256> approx_log(VectorRegister<double, 256> const &x)
{
  enum
  {
    mantissa = 20,
    exponent = 11,
  };

  alignas(32) constexpr uint64_t    mask[4]         = {uint64_t(-1), 0, 0, 0};
  constexpr auto                    multiplier      = double(1ull << mantissa);
  constexpr double                  exponent_offset = (double(((1ull << (exponent - 1)) - 1)));
  const VectorRegister<double, 256> a(double(M_LN2 / multiplier));
  const VectorRegister<double, 256> b(double(exponent_offset * multiplier - 60801));

  __m256i conv = _mm256_castpd_si256(x.data());
  conv         = _mm256_shuffle_epi32(conv, 1 | (3 << 2) | (0 << 4) | (2 << 6));
  conv         = _mm256_and_si256(conv, *reinterpret_cast<__m256i const *>(mask));

  // Note: _mm256_cvtepi32_pd returns a __m128i, fix
  VectorRegister<double, 256> y(_mm256_castsi256_pd(conv));

  return a * (y - b);
}

}  // namespace vectorise
}  // namespace fetch
