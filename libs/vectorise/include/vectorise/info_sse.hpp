#pragma once

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
