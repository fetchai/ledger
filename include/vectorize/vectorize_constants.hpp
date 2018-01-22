#ifndef VETORIZE_VECTORIZE_CONSTANTS_HPP
#define VETORIZE_VECTORIZE_CONSTANTS_HPP
#include<cstdint>
namespace fetch {
namespace vectorize {

enum InstructionSet {
  NO_VECTOR = 0,
  X86_SSE3 = 1,
  X86_SSE4 = 3,
  X86_AVX = 7,
  X86_AVX2 = 15,
  ARM_NEON = 16
};
  
template <uint16_t I, typename T>
struct RegisterInfo {
  enum { size = sizeof(T), alignment = 16 };
};

template <typename T>
struct RegisterInfo<X86_SSE3, T> {
  enum { size = 16, alignment = 16 };
};
template <typename T>
struct RegisterInfo<X86_SSE4, T> {
  enum { size = 16, alignment = 16 };
};
template <typename T>
struct RegisterInfo<X86_AVX, T> {
  enum { size = 32, alignment = 16 };
};
template <typename T>
struct RegisterInfo<X86_AVX2, T> {
  enum { size = 32, alignment = 16 };
};
template <typename T>
struct RegisterInfo<ARM_NEON, T> {
  enum { size = 16, alignment = 16 };
};
  
  
};
};

#endif
