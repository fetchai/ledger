#ifndef PLATFORM_HPP
#define PLATFORM_HPP
#include "vectorize/sse.hpp"

namespace fetch {
namespace platform {
  enum {
#ifdef __AVX__    
    vector_size = 256
#elif defined __SSE__
    vector_size = 128
#else
    vector_size = 32
#endif
  };
  
  
constexpr bool has_avx() {
#ifdef __AVX__
  return true;
#else
  return false;
#endif
}

constexpr bool has_avx2() {
#ifdef __AVX2__
  return true;
#else
  return false;
#endif

}

constexpr bool has_sse() {
#ifdef __SSE__
  return true;
#else
  return false;
#endif
};


constexpr bool has_sse2() {
#ifdef __SSE2__
  return true;
#else
  return false;
#endif
};

constexpr bool has_sse3() {
#ifdef __SSE3__
  return true;
#else
  return false;
#endif
};

constexpr bool has_sse41() {
#ifdef __SSE4_1__
  return true;
#else
  return false;
#endif
};
  
constexpr bool has_sse42() {
#ifdef __SSE4_2__
  return true;
#else
  return false;
#endif
};
  
};
};

#endif
