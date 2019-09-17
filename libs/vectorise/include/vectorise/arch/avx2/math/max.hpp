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

#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace vectorise {

inline VectorRegister<int32_t, 128> Max(VectorRegister<int32_t, 128> const &a,
                                        VectorRegister<int32_t, 128> const &b)
{
  return VectorRegister<int32_t, 128>(_mm_max_epi32(a.data(), b.data()));
}

inline VectorRegister<int32_t, 256> Max(VectorRegister<int32_t, 256> const &a,
                                        VectorRegister<int32_t, 256> const &b)
{
  return VectorRegister<int32_t, 256>(_mm256_max_epi32(a.data(), b.data()));
}

inline VectorRegister<int64_t, 128> Max(VectorRegister<int64_t, 128> const &a,
                                        VectorRegister<int64_t, 128> const &b)
{
  __m128i mask = (a > b).data();
  __m128i ret  = _mm_blendv_epi8(b.data(), a.data(), mask);
  return VectorRegister<int64_t, 128>(ret);
}

inline VectorRegister<int64_t, 256> Max(VectorRegister<int64_t, 256> const &a,
                                        VectorRegister<int64_t, 256> const &b)
{
  __m256i mask = (a > b).data();
  __m256i ret  = _mm256_blendv_epi8(b.data(), a.data(), mask);
  return VectorRegister<int64_t, 256>(ret);
}

inline VectorRegister<fixed_point::fp32_t, 128> Max(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  VectorRegister<int32_t, 128> max_int32 =
      Max(VectorRegister<int32_t, 128>(a.data()), VectorRegister<int32_t, 128>(b.data()));
  return VectorRegister<fixed_point::fp32_t, 128>(max_int32.data());
}

inline VectorRegister<fixed_point::fp32_t, 256> Max(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{
  VectorRegister<int32_t, 256> max_int32 =
      Max(VectorRegister<int32_t, 256>(a.data()), VectorRegister<int32_t, 256>(b.data()));
  return VectorRegister<fixed_point::fp32_t, 256>(max_int32.data());
}

inline VectorRegister<fixed_point::fp64_t, 128> Max(
    VectorRegister<fixed_point::fp64_t, 128> const &a,
    VectorRegister<fixed_point::fp64_t, 128> const &b)
{
  VectorRegister<int64_t, 128> max_int64 =
      Max(VectorRegister<int64_t, 128>(a.data()), VectorRegister<int64_t, 128>(b.data()));
  return VectorRegister<fixed_point::fp64_t, 128>(max_int64.data());
}

inline VectorRegister<fixed_point::fp64_t, 256> Max(
    VectorRegister<fixed_point::fp64_t, 256> const &a,
    VectorRegister<fixed_point::fp64_t, 256> const &b)
{
  VectorRegister<int64_t, 256> max_int64 =
      Max(VectorRegister<int64_t, 256>(a.data()), VectorRegister<int64_t, 256>(b.data()));
  return VectorRegister<fixed_point::fp64_t, 256>(max_int64.data());
}

inline VectorRegister<float, 128> Max(VectorRegister<float, 128> const &a,
                                      VectorRegister<float, 128> const &b)
{
  return VectorRegister<float, 128>(_mm_max_ps(a.data(), b.data()));
}

inline VectorRegister<float, 256> Max(VectorRegister<float, 256> const &a,
                                      VectorRegister<float, 256> const &b)
{
  return VectorRegister<float, 256>(_mm256_max_ps(a.data(), b.data()));
}

inline VectorRegister<double, 128> Max(VectorRegister<double, 128> const &a,
                                       VectorRegister<double, 128> const &b)
{
  return VectorRegister<double, 128>(_mm_max_pd(a.data(), b.data()));
}

inline VectorRegister<double, 256> Max(VectorRegister<double, 256> const &a,
                                       VectorRegister<double, 256> const &b)
{
  return VectorRegister<double, 256>(_mm256_max_pd(a.data(), b.data()));
}

}  // namespace vectorise
}  // namespace fetch
