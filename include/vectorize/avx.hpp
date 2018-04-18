#ifndef VECTORIZE_AVX_HPP
#define VECTORIZE_AVX_HPP
#include "vectorize/info.hpp"
#include "vectorize/register.hpp"
#include "vectorize/info_avx.hpp"

#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#include <cstdint>
#include <cstddef>


namespace fetch {
namespace vectorize {
  /*
  namespace details {
    template< typename T, std::size_t N >
    struct UnrollSet {
      static void Set(T *ptr, T const &c) {
        (*ptr)  = c;
        UnrollSet<T, N - 1>::Set(ptr+1, c);
      }
    };
    
    template<typename T >
    struct UnrollSet<T,0> {
      static void Set(T *ptr, T const &c) { }
    };
  };
  */
// SSE integers
template <typename T>
class VectorRegister<T, 256, __m256i> {
 public:
  typedef T type;
  typedef __m256i mm_register_type;
  
  enum {
    E_VECTOR_SIZE = 256,    
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT = E_REGISTER_SIZE / sizeof(type)
  };
  
  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) { data_ = _mm256_load_si256((mm_register_type *)d); }
  VectorRegister(type const &c)  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet< type, E_BLOCK_COUNT >::Set(constant, c);
    data_ = _mm256_load_si256((mm_register_type *)constant);
    
  }
  VectorRegister(mm_register_type const &d) : data_(d) {}
  VectorRegister(mm_register_type &&d) : data_(d) {}

  explicit operator mm_register_type() { return data_; }

  void Store(type *ptr) const { _mm256_store_si256((mm_register_type *)ptr, data_); }
  void Stream(type *ptr) const { _mm256_stream_si256((mm_register_type *)ptr, data_); }
  
  mm_register_type const &data() const { return data_; }
  mm_register_type &data()  { return data_; }  
 private:
  mm_register_type data_;

};


template <>
class VectorRegister<float, 256, __m256> {
 public:
  typedef float type;
  typedef __m256 mm_register_type;
  
  enum {
    E_VECTOR_SIZE = 256,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT = E_REGISTER_SIZE / sizeof(type)
  };
  
  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) { data_ = _mm256_load_ps(d); }
  VectorRegister(mm_register_type const &d) : data_(d) {}
  VectorRegister(mm_register_type &&d) : data_(d) {}
  VectorRegister(type const &c)  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet< type, E_BLOCK_COUNT >::Set(constant, c);
    data_ = _mm256_load_ps(constant);
    
  }
  
  explicit operator mm_register_type() { return data_; }

  void Store(type *ptr) const { _mm256_store_ps(ptr, data_); }
  void Stream(type *ptr) const { _mm256_stream_ps(ptr, data_); }

  mm_register_type const &data() const { return data_; }
  mm_register_type &data()  { return data_; }  
 private:
  mm_register_type data_;
};
  
template <>
class VectorRegister<double, 256, __m256d> {
 public:
  typedef double type;
  typedef __m256d mm_register_type;
  
  enum {
    E_VECTOR_SIZE = 256,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT = E_REGISTER_SIZE / sizeof(type)
  };
  
  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) { data_ = _mm256_load_pd(d); }
  VectorRegister(mm_register_type const &d) : data_(d) {}
  VectorRegister(mm_register_type &&d) : data_(d) {}
  VectorRegister(type const &c)  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet< type, E_BLOCK_COUNT >::Set(constant, c);
    data_ = _mm256_load_pd(constant);
    
  }
  
  explicit operator mm_register_type() { return data_; }

  void Store(type *ptr) const { _mm256_stream_pd(ptr, data_); }
  void Stream(type *ptr) const { _mm256_stream_pd(ptr, data_); }
  
  mm_register_type const &data() const { return data_; }
  mm_register_type &data()  { return data_; }  
 private:
  mm_register_type data_;
};


#define FETCH_ADD_OPERATOR(op, type, L, fnc)                            \
  inline VectorRegister<type, 256, L>                                   \
  operator op( VectorRegister<type,256, L> const &a,                    \
               VectorRegister<type,256, L> const &b) {                  \
    L ret = fnc(a.data(), b.data());                                    \
    return VectorRegister<type, 256, L>(ret);                           \
  }

#ifdef __AVX2__
FETCH_ADD_OPERATOR(*, int, __m256i, _mm256_mullo_epi32);
FETCH_ADD_OPERATOR(-, int, __m256i, _mm256_sub_epi32);
//FETCH_ADD_OPERATOR(/, int, __m256i, _mm256_div_epi32);
FETCH_ADD_OPERATOR(+, int, __m256i, _mm256_add_epi32);  
#endif

FETCH_ADD_OPERATOR(*, float, __m256, _mm256_mul_ps);
FETCH_ADD_OPERATOR(-, float, __m256, _mm256_sub_ps);  
FETCH_ADD_OPERATOR(/, float, __m256, _mm256_div_ps);
FETCH_ADD_OPERATOR(+, float, __m256, _mm256_add_ps);  

FETCH_ADD_OPERATOR(*, double, __m256d, _mm256_mul_pd);
FETCH_ADD_OPERATOR(-, double, __m256d, _mm256_sub_pd);  
FETCH_ADD_OPERATOR(/, double, __m256d, _mm256_div_pd);
FETCH_ADD_OPERATOR(+, double, __m256d, _mm256_add_pd);  
  
  
#undef FETCH_ADD_OPERATOR

};
};
#endif
