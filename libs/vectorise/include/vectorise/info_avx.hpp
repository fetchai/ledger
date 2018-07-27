#pragma once
#ifdef __AVX__
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorize {

template <>
struct VectorInfo<uint8_t, 256>
{
  using naitve_type = uint8_t;
  using register_type = __m256i;
};

template <>
struct VectorInfo<uint16_t, 256>
{
  using naitve_type = uint16_t;
  using register_type = __m256i;
};

template <>
struct VectorInfo<uint32_t, 256>
{
  using naitve_type = uint32_t;
  using register_type = __m256i;
};

template <>
struct VectorInfo<uint64_t, 256>
{
  using naitve_type = uint64_t;
  using register_type = __m256i;
};

template <>
struct VectorInfo<int, 256>
{
  using naitve_type = int;
  using register_type = __m256i;
};

template <>
struct VectorInfo<float, 256>
{
  using naitve_type = float;
  using register_type = __m256;
};

template <>
struct VectorInfo<double, 256>
{
  using naitve_type = double;
  using register_type = __m256d;
};
}  // namespace vectorize
}  // namespace fetch
#endif
