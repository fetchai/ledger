#ifndef VECTORIZE_MATH_APPROX_LOG_HPP
#define VECTORIZE_MATH_APPROX_LOG_HPP
namespace fetch {
namespace vectorize {


inline VectorRegister<float, 128> approx_log( VectorRegister<float, 128> const &x) {
  enum {
    mantissa = 23,
    exponent = 8,
  };

  constexpr float multiplier =  float( 1ull << mantissa  );
  constexpr float exponent_offset = ( float( ((1ull << (exponent - 1)) - 1) ) );
  const VectorRegister<float, 128> a( float( M_LN2 / multiplier ) );
  const VectorRegister<float, 128> b( float( exponent_offset * multiplier - 60801 ));
  
  __m128i conv = _mm_castps_si128( x.data() ); 

  VectorRegister<float, 128> y( _mm_cvtepi32_ps(conv) );
  
  return  a*(y - b);
}

inline VectorRegister<double, 128> approx_log( VectorRegister<double, 128> const &x) {
  enum {
    mantissa = 20,
    exponent = 11,
  };

  alignas(16) constexpr uint64_t mask[2]= { uint64_t(-1), 0 }; 
  constexpr double multiplier =  double( 1ull << mantissa  );
  constexpr double exponent_offset = ( double( ((1ull << (exponent - 1)) - 1) ) );
  const VectorRegister<double, 128> a( double( M_LN2 / multiplier ) );
  const VectorRegister<double, 128> b( double( exponent_offset * multiplier - 60801 ));

  __m128i conv = _mm_castpd_si128( x.data() ); 
  conv = _mm_shuffle_epi32(conv,  1 | (3 << 2) | (0<<4) | (2 << 6) );
  conv = _mm_and_si128(conv, *(__m128i*)mask);

  VectorRegister<double, 128> y( _mm_cvtepi32_pd(conv) );
  
  return  a*(y - b);
}


}
}

#endif
