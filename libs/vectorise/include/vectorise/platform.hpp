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

#include "meta/type_traits.hpp"

#if (__SIZEOF_INT128__ == 16)
using int128_t  = __int128_t;
using uint128_t = __uint128_t;
#endif

namespace fetch {
namespace platform {

inline uint8_t ToBigEndian(uint8_t x)
{
  return x;
}

inline uint8_t FromBigEndian(uint8_t x)
{
  return x;
}

inline uint16_t ToBigEndian(uint16_t x)
{
  return __builtin_bswap16(x);
}

inline uint16_t FromBigEndian(uint16_t x)
{
  return __builtin_bswap16(x);
}

inline uint32_t ToBigEndian(uint32_t x)
{
  return __builtin_bswap32(x);
}

inline uint32_t FromBigEndian(uint32_t x)
{
  return __builtin_bswap32(x);
}

inline uint64_t ToBigEndian(uint64_t x)
{
  return __builtin_bswap64(x);
}

inline uint64_t FromBigEndian(uint64_t x)
{
  return __builtin_bswap64(x);
}

inline int8_t ToBigEndian(int8_t x)
{
  return x;
}

inline int8_t FromBigEndian(int8_t x)
{
  return x;
}

inline int16_t ToBigEndian(int16_t x)
{
  return static_cast<int16_t>(__builtin_bswap16(static_cast<uint16_t>(x)));
}

inline int16_t FromBigEndian(int16_t x)
{
  return static_cast<int16_t>(__builtin_bswap16(static_cast<uint16_t>(x)));
}

inline int32_t ToBigEndian(int32_t x)
{
  return static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(x)));
}

inline int32_t FromBigEndian(int32_t x)
{
  return static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(x)));
}

inline int64_t ToBigEndian(int64_t x)
{
  return static_cast<int64_t>(__builtin_bswap64(static_cast<uint64_t>(x)));
}

inline int64_t FromBigEndian(int64_t x)
{
  return static_cast<int64_t>(__builtin_bswap64(static_cast<uint64_t>(x)));
}

inline float ToBigEndian(float x)
{
  union
  {
    float    value;
    uint32_t bytes;
  } conversion{};
  static_assert(sizeof(float) == sizeof(uint32_t),
                "float and uint32_t are required to be same size.");
  static_assert(sizeof(conversion) == sizeof(uint32_t), "");

  conversion.value = x;
  conversion.bytes = __builtin_bswap32(conversion.bytes);
  return conversion.value;
}

inline float FromBigEndian(float x)
{
  union
  {
    float    value;
    uint32_t bytes;
  } conversion{};
  static_assert(sizeof(float) == sizeof(uint32_t),
                "float and uint32_t are required to be same size.");
  static_assert(sizeof(conversion) == sizeof(uint32_t), "");

  conversion.value = x;
  conversion.bytes = __builtin_bswap32(conversion.bytes);
  return conversion.value;
}

inline double ToBigEndian(double x)
{
  union
  {
    double   value;
    uint64_t bytes;
  } conversion{};
  static_assert(sizeof(double) == sizeof(uint64_t),
                "double and uint64_t are required to be same size.");
  static_assert(sizeof(conversion) == sizeof(uint64_t), "");

  conversion.value = x;
  conversion.bytes = __builtin_bswap64(conversion.bytes);
  return conversion.value;
}

inline double FromBigEndian(double x)
{
  union
  {
    double   value;
    uint64_t bytes;
  } conversion{};
  static_assert(sizeof(double) == sizeof(uint64_t),
                "double and uint64_t are required to be same size.");
  static_assert(sizeof(conversion) == sizeof(uint64_t), "");

  conversion.value = x;
  conversion.bytes = __builtin_bswap64(conversion.bytes);
  return conversion.value;
}

struct Parallelisation
{
  enum
  {
    NOT_PARALLEL = 0,
    VECTORISE    = 1,
    THREADING    = 2
  };
};

constexpr bool has_avx2()
{
#ifdef __AVX2__
  return true;
#else
  return false;
#endif
}

#define FETCH_ASM_LABEL(Label) __asm__("#" Label)

// Allow the option of specifying our platform endianness
#if defined(FETCH_PLATFORM_BIG_ENDIAN) || defined(FETCH_PLATFORM_LITTLE_ENDIAN)
#else

#if (defined(__BYTE_ORDER) && (__BYTE_ORDER == __BIG_ENDIAN)) ||             \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)) || \
    defined(__BIG_ENDIAN__)

#define FETCH_PLATFORM_BIG_ENDIAN

#elif (defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN)) ||           \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)) || \
    defined(__LITTLE_ENDIAN__)

#define FETCH_PLATFORM_LITTLE_ENDIAN

#else
#error "Can't determine machine endianness"
#endif

#endif

#if defined(FETCH_PLATFORM_BIG_ENDIAN)
inline uint64_t ConvertToBigEndian(uint64_t x)
{
  return x;
}
#endif

#if defined(FETCH_PLATFORM_LITTLE_ENDIAN)
inline uint64_t ConvertToBigEndian(uint64_t x)
{
  return __builtin_bswap64(x);
}
#endif

inline uint64_t CountLeadingZeroes64(uint64_t x)
{
  return x == 0 ? 64 : static_cast<uint64_t>(__builtin_clzll(x));
}

inline uint64_t CountTrailingZeroes64(uint64_t x)
{
  return x == 0 ? 64 : static_cast<uint64_t>(__builtin_ctzll(x));
}

inline uint64_t CountSetBits(uint64_t x)
{
  return static_cast<uint64_t>(__builtin_popcountll(x));
}

/**
 * finds most significant set bit in type
 * @tparam T
 * @param n
 * @return
 */
template <typename T>
constexpr int32_t HighestSetBit(T n_input)
{
  auto const n = static_cast<uint64_t>(n_input);

  if (n == 0)
  {
    return 0;
  }

  return static_cast<int32_t>((sizeof(uint64_t) * 8)) -
         static_cast<int32_t>(CountLeadingZeroes64(n));
}

/**
 * finds most significant set bit in type
 * @tparam T
 * @param n
 * @return
 */
template <>
constexpr int32_t HighestSetBit(int128_t n_input)
{
  auto const high = static_cast<uint64_t>(n_input >> 64);
  auto const low  = static_cast<uint64_t>(n_input >> 64);

  if (high == 0)
  {
    return HighestSetBit(low);
  }
  return HighestSetBit(high) + 64;
}

// Return the minimum number of bits required to represent x
inline uint64_t Log2Ceil(uint64_t x)
{
  uint64_t count = 0;
  while (x >>= 1)  // NOLINT
  {
    count++;
  }

  if ((1ull << count) == x)
  {
    return count;
  }

  return count + 1;
}

inline uint32_t ToLog2(uint32_t value)
{
  static constexpr uint32_t VALUE_SIZE_IN_BITS = sizeof(value) << 3u;
  return static_cast<uint32_t>(VALUE_SIZE_IN_BITS -
                               static_cast<uint32_t>(__builtin_clz(value) + 1));
}

inline uint64_t ToLog2(uint64_t value)
{
  static constexpr uint64_t VALUE_SIZE_IN_BITS = sizeof(value) << 3u;
  return VALUE_SIZE_IN_BITS - static_cast<uint64_t>(__builtin_clzll(value) + 1);
}

// https://graphics.stanford.edu/~seander/bithacks.html
inline bool IsLog2(uint64_t value)
{
  return (value != 0u) && ((value & (value - 1)) == 0u);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
T DivideCeil(T x, T y)
{
  T ret = x / y;

  if (y * ret < x)
  {
    ++ret;
  }

  return ret;
}

template <uint32_t DIVISOR, typename Value>
meta::IfIsUnsignedInteger<Value, Value> DivCeil(Value value)
{
  static_assert(DIVISOR > 0, "Static asserts can only protect you so far");

  static constexpr Value ADDITION = DIVISOR - 1u;

  return (value + ADDITION) / DIVISOR;
}

}  // namespace platform
}  // namespace fetch
