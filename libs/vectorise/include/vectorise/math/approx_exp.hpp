#ifndef VECTORIZE_MATH_APPROX_EXP_HPP
#define VECTORIZE_MATH_APPROX_EXP_HPP
namespace fetch {
namespace vectorize {

inline VectorRegister<float, 128> approx_exp(VectorRegister<float, 128> const &x) {
  enum {
    mantissa = 23,
    exponent = 8,
  };

  constexpr float multiplier =  float( 1ull << mantissa  );
  constexpr float exponent_offset = ( float( ((1ull << (exponent - 1)) - 1) ) );
  const VectorRegister<float, 128> a( float( multiplier / M_LN2 ) );
  const VectorRegister<float, 128> b( float( exponent_offset * multiplier - 60801 ));

  VectorRegister<float, 128> y = a*x + b;
  __m128i  conv = _mm_cvtps_epi32(y.data());

  return VectorRegister<float, 128>( _mm_castsi128_ps(conv) );
}

inline VectorRegister<double, 128> approx_exp( VectorRegister<double, 128> const &x) {
  enum {
    mantissa = 20,
    exponent = 11,
  };

  alignas(16) constexpr uint64_t mask[2]= { uint64_t(-1), 0 }; 
  constexpr double multiplier =  double( 1ull << mantissa  );
  constexpr double exponent_offset = ( double( ((1ull << (exponent - 1)) - 1) ) );
  const VectorRegister<double, 128> a( double( multiplier / M_LN2 ) );
  const VectorRegister<double, 128> b( double( exponent_offset * multiplier - 60801 ));

  VectorRegister<double, 128> y = a*x + b;
  __m128i  conv = _mm_cvtpd_epi32(y.data());
  conv = _mm_and_si128(conv, *(__m128i*)mask);  

  conv = _mm_shuffle_epi32(conv,  3 | (0 << 2) | (3<<4) | (1 << 6) );

  return VectorRegister<double, 128>( _mm_castsi128_pd(conv) );
}


}
}

#endif
