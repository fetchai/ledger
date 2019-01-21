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

#include "vectorise/vectorise.hpp"

namespace fetch {
namespace platform {

struct Parallelisation
{
  enum
  {
    NOT_PARALLEL = 0,
    VECTORISE    = 1,
    THREADING    = 2
  };
};

template <typename T>
struct VectorRegisterSize
{
  enum
  {
#ifdef __AVX__
    value = 256
#elif defined __SSE__
    value = 128
#else
    value = 32
#endif
  };
};

#define ADD_REGISTER_SIZE(type, size) \
  template <>                         \
  struct VectorRegisterSize<type>     \
  {                                   \
    enum                              \
    {                                 \
      value = (size)                  \
    };                                \
  }

#ifdef __AVX2__

ADD_REGISTER_SIZE(int, 256);

#elif defined __AVX__

ADD_REGISTER_SIZE(int, 128);
ADD_REGISTER_SIZE(double, 256);
ADD_REGISTER_SIZE(float, 256);

#elif defined __SSE42__

ADD_REGISTER_SIZE(int, 128);
ADD_REGISTER_SIZE(double, 128);
ADD_REGISTER_SIZE(float, 128);

#elif defined __SSE3__

ADD_REGISTER_SIZE(int, 128);
ADD_REGISTER_SIZE(double, 128);
ADD_REGISTER_SIZE(float, 128);

#else

ADD_REGISTER_SIZE(int, sizeof(int));

#endif
#undef ADD_REGISTER_SIZE

constexpr bool has_avx()
{
#ifdef __AVX__
  return true;
#else
  return false;
#endif
}

constexpr bool has_avx2()
{
#ifdef __AVX2__
  return true;
#else
  return false;
#endif
}

constexpr bool has_sse()
{
#ifdef __SSE__
  return true;
#else
  return false;
#endif
}

constexpr bool has_sse2()
{
#ifdef __SSE2__
  return true;
#else
  return false;
#endif
}

constexpr bool has_sse3()
{
#ifdef __SSE3__
  return true;
#else
  return false;
#endif
}

constexpr bool has_sse41()
{
#ifdef __SSE4_1__
  return true;
#else
  return false;
#endif
}

constexpr bool has_sse42()
{
#ifdef __SSE4_2__
  return true;
#else
  return false;
#endif
}

// Allow the option of specifying our platform endianness
#if defined(FETCH_PLATFORM_BIG_ENDIAN) || defined(FETCH_PLATFORM_LITTLE_ENDIAN)
#else

#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || defined(__BIG_ENDIAN__)

#define FETCH_PLATFORM_BIG_ENDIAN

#elif (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)

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

inline int CountLeadingZeroes64(uint64_t x)
{
  return __builtin_clzl(x);
}

// Return the minimum number of bits required to represent x
inline uint64_t Log2Ceil(uint64_t x)
{
  uint64_t count = 0;
  while (x >>= 1)
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
  static constexpr uint32_t VALUE_SIZE_IN_BITS = sizeof(value) << 3;
  return static_cast<uint32_t>(VALUE_SIZE_IN_BITS -
                               static_cast<uint32_t>(__builtin_clz(value) + 1));
}

}  // namespace platform
}  // namespace fetch
