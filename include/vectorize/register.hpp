#ifndef VECTORIZE_REGISTER_HPP
#define VECTORIZE_REGISTER_HPP
#include "vectorize/vectorize_constants.hpp"

#include <emmintrin.h>

#include <smmintrin.h>
#include <type_traits>
#include <typeinfo>
#include<iostream>
namespace fetch {
namespace vectorize {

  template< typename T, std::size_t >
  struct VectorInfo {
    typedef T naitve_type;
    typedef T register_type;
  };

  template<>
  struct VectorInfo< uint8_t, 128 > {
    typedef uint8_t naitve_type;
    typedef __m128i register_type;
  };
  
  template<>
  struct VectorInfo< uint16_t, 128 > {
    typedef uint16_t naitve_type;
    typedef __m128i register_type;
  };

  template<>
  struct VectorInfo< uint32_t, 128 > {
    typedef uint32_t naitve_type;
    typedef __m128i register_type;
  };

  template<>
  struct VectorInfo< uint64_t, 128 > {
    typedef uint64_t naitve_type;
    typedef __m128i register_type;
  };

  template<>
  struct VectorInfo< float, 128 > {
    typedef float naitve_type;
    typedef __m128 register_type;
  };

  template<>
  struct VectorInfo< double, 128 > {
    typedef double naitve_type;
    typedef __m128 register_type;
  };

  
  template <typename T, std::size_t N, typename S = typename VectorInfo<T,N>::register_type  >
class VectorRegister {
 public:
  typedef T type;
  typedef S mm_register_type;
  
  enum {
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT = E_REGISTER_SIZE / sizeof(type)
  };
  
  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) : data_(*d) {}
  VectorRegister(type const &d) : data_(d) {}
  VectorRegister(type &&d) : data_(d) {}

  explicit operator T() { return data_; }

#define AILIB_ADD_OPERATOR(OP)                              \
  VectorRegister operator OP(VectorRegister const &other) { \
    return VectorRegister(data_ OP other.data_);            \
  }
  // APPLY_OPERATOR_LIST(AILIB_ADD_OPERATOR);
#undef AILIB_ADD_OPERATOR

  void Store(type *ptr) const { *ptr = data_; }

 private:
  type data_;
};

};
};
#endif
