#ifndef VECTORIZE_INFO_AVX_HPP
#define VECTORIZE_INFO_AVX_HPP
#ifdef __AVX__
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#include <cstdint>
#include <cstddef>

namespace fetch {
namespace vectorize {

template<>
struct VectorInfo< uint8_t, 256 > {
  typedef uint8_t naitve_type;
  typedef __m256i register_type;
};

template<>
struct VectorInfo< uint16_t, 256 > {
  typedef uint16_t naitve_type;
  typedef __m256i register_type;
};

template<>
struct VectorInfo< uint32_t, 256 > {
  typedef uint32_t naitve_type;
  typedef __m256i register_type;
};

template<>
struct VectorInfo< uint64_t, 256 > {
  typedef uint64_t naitve_type;
  typedef __m256i register_type;
};

template<>
struct VectorInfo< int, 256 > {
  typedef int naitve_type;
  typedef __m256i register_type;
};


template<>
struct VectorInfo< float, 256 > {
  typedef float naitve_type;
  typedef __m256 register_type;
};

template<>
struct VectorInfo< double, 256 > {
  typedef double naitve_type;
  typedef __m256d register_type;
};


  

}
}
#endif
#endif
