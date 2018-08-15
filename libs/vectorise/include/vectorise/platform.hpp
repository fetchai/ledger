#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
}  // namespace platform
}  // namespace fetch
