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

#include "vectorise/info.hpp"
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorize {

template <>
struct VectorInfo<uint8_t, 128>
{
  using naitve_type   = uint8_t;
  using register_type = __m128i;
};

template <>
struct VectorInfo<uint16_t, 128>
{
  using naitve_type   = uint16_t;
  using register_type = __m128i;
};

template <>
struct VectorInfo<uint32_t, 128>
{
  using naitve_type   = uint32_t;
  using register_type = __m128i;
};

template <>
struct VectorInfo<uint64_t, 128>
{
  using naitve_type   = uint64_t;
  using register_type = __m128i;
};

template <>
struct VectorInfo<int, 128>
{
  using naitve_type   = int;
  using register_type = __m128i;
};

template <>
struct VectorInfo<float, 128>
{
  using naitve_type   = float;
  using register_type = __m128;
};

template <>
struct VectorInfo<double, 128>
{
  using naitve_type   = double;
  using register_type = __m128d;
};
}  // namespace vectorize
}  // namespace fetch
