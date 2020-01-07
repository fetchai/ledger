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

#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace vectorise {

inline VectorRegister<int32_t, 128> Min(VectorRegister<int32_t, 128> const &a,
                                        VectorRegister<int32_t, 128> const &b)
{
  auto const ret = VectorRegister<int32_t, 128>(_mm_min_epi32(a.data(), b.data()));
  return ret;
}

inline VectorRegister<int32_t, 256> Min(VectorRegister<int32_t, 256> const &a,
                                        VectorRegister<int32_t, 256> const &b)
{
  auto const ret = VectorRegister<int32_t, 256>(_mm256_min_epi32(a.data(), b.data()));
  return ret;
}

inline VectorRegister<int64_t, 128> Min(VectorRegister<int64_t, 128> const &a,
                                        VectorRegister<int64_t, 128> const &b)
{
  __m128i    mask = (a > b).data();
  auto const ret  = VectorRegister<int64_t, 128>(_mm_blendv_epi8(a.data(), b.data(), mask));
  return ret;
}

inline VectorRegister<int64_t, 256> Min(VectorRegister<int64_t, 256> const &a,
                                        VectorRegister<int64_t, 256> const &b)
{
  __m256i    mask = (a > b).data();
  auto const ret  = VectorRegister<int64_t, 256>(_mm256_blendv_epi8(a.data(), b.data(), mask));
  return ret;
}

inline VectorRegister<fixed_point::fp32_t, 128> Min(
    VectorRegister<fixed_point::fp32_t, 128> const &a,
    VectorRegister<fixed_point::fp32_t, 128> const &b)
{
  VectorRegister<int32_t, 128> min_int32 =
      Min(VectorRegister<int32_t, 128>(a.data()), VectorRegister<int32_t, 128>(b.data()));
  auto const ret = VectorRegister<fixed_point::fp32_t, 128>(min_int32.data());
  return ret;
}

inline VectorRegister<fixed_point::fp32_t, 256> Min(
    VectorRegister<fixed_point::fp32_t, 256> const &a,
    VectorRegister<fixed_point::fp32_t, 256> const &b)
{
  VectorRegister<int32_t, 256> min_int32 =
      Min(VectorRegister<int32_t, 256>(a.data()), VectorRegister<int32_t, 256>(b.data()));
  auto const ret = VectorRegister<fixed_point::fp32_t, 256>(min_int32.data());
  return ret;
}

inline VectorRegister<fixed_point::fp64_t, 128> Min(
    VectorRegister<fixed_point::fp64_t, 128> const &a,
    VectorRegister<fixed_point::fp64_t, 128> const &b)
{
  VectorRegister<int64_t, 128> min_int64 =
      Min(VectorRegister<int64_t, 128>(a.data()), VectorRegister<int64_t, 128>(b.data()));
  auto const ret = VectorRegister<fixed_point::fp64_t, 128>(min_int64.data());
  return ret;
}

inline VectorRegister<fixed_point::fp64_t, 256> Min(
    VectorRegister<fixed_point::fp64_t, 256> const &a,
    VectorRegister<fixed_point::fp64_t, 256> const &b)
{
  VectorRegister<int64_t, 256> min_int64 =
      Min(VectorRegister<int64_t, 256>(a.data()), VectorRegister<int64_t, 256>(b.data()));
  auto const ret = VectorRegister<fixed_point::fp64_t, 256>(min_int64.data());
  return ret;
}

inline VectorRegister<float, 128> Min(VectorRegister<float, 128> const &a,
                                      VectorRegister<float, 128> const &b)
{
  auto const ret = VectorRegister<float, 128>(_mm_min_ps(a.data(), b.data()));
  return ret;
}

inline VectorRegister<float, 256> Min(VectorRegister<float, 256> const &a,
                                      VectorRegister<float, 256> const &b)
{
  auto const ret = VectorRegister<float, 256>(_mm256_min_ps(a.data(), b.data()));
  return ret;
}

inline VectorRegister<double, 128> Min(VectorRegister<double, 128> const &a,
                                       VectorRegister<double, 128> const &b)
{
  auto const ret = VectorRegister<double, 128>(_mm_min_pd(a.data(), b.data()));
  return ret;
}

inline VectorRegister<double, 256> Min(VectorRegister<double, 256> const &a,
                                       VectorRegister<double, 256> const &b)
{
  auto const ret = VectorRegister<double, 256>(_mm256_min_pd(a.data(), b.data()));
  return ret;
}

}  // namespace vectorise
}  // namespace fetch
