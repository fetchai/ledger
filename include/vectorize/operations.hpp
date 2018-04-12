#ifndef VECTORIZE_OPERATIONS_HPP
#define  VECTORIZE_OPERATIONS_HPP
#include<xmmintrin.h>
namespace fetch {
  namespace vectorize {
    template< char operation, typename T, std::size_t instruction >
    struct VectorOperation { };


#define DEFINE_VECTOR_OPERATION(CID, NATIVE_TYPE, SIZE, REG,CAST, LOAD, STORE, APPLY ) \
    template<>                                                          \
    struct VectorOperation<CID,NATIVE_TYPE, SIZE> { \
      typedef NATIVE_TYPE native_type ;            \
      typedef REG register_type;            \
                                                                        \
      static void Apply(native_type const *a, native_type const * b, native_type* c) { \
        register_type ma, mb, mc;                                       \
        Load(a, ma);                                                    \
        Load(b, mb);                                                    \
        mc = Apply(ma,mb);                                               \
        Store(c, mc);                                                   \
      }                                                                 \
                                                                        \
      static register_type Apply(register_type const&a, register_type const& b) {\
        return  APPLY(a,b);                                        \
      }                                                                 \
                                                                        \
      static void Load(native_type const *ptr, register_type &ret) {    \
        ret = LOAD( CAST ptr);                                         \
      }                                                                 \
                                                                        \
      static void Store(native_type *ptr, register_type &data) {        \
        STORE(CAST ptr, data);                                             \
      }                                                                 \
}


    // Ints
    DEFINE_VECTOR_OPERATION('*', uint64_t, 128, __m128i, (__m128i*), _mm_load_si128, _mm_store_si128, _mm_mul_epu32);


    // Floats
    DEFINE_VECTOR_OPERATION('*', float, 128, __m128,,  _mm_load_ps,  _mm_store_ps, _mm_mul_ps);
    DEFINE_VECTOR_OPERATION('+', float, 128, __m128,,  _mm_load_ps,  _mm_store_ps, _mm_add_ps);
    DEFINE_VECTOR_OPERATION('/', float, 128, __m128,,  _mm_load_ps,  _mm_store_ps, _mm_div_ps);
    DEFINE_VECTOR_OPERATION('-', float, 128, __m128,,  _mm_load_ps,  _mm_store_ps, _mm_sub_ps);

    // Doubles
    DEFINE_VECTOR_OPERATION('*', double, 128, __m128,,  _mm_load_pd,  _mm_store_pd, _mm_mul_pd);

  };
};

#endif
