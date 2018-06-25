#ifndef VECTORIZE_MATH_SQRT_HPP
#define VECTORIZE_MATH_SQRT_HPP
namespace fetch {
namespace vectorize {

inline VectorRegister<float, 128> sqrt(VectorRegister<float, 128> const &a) {
  return VectorRegister<float, 128>( _mm_sqrt_ps(a.data() ) );
}

inline VectorRegister<double, 128> sqrt( VectorRegister<double, 128> const &a) {
  return VectorRegister<double, 128>( _mm_sqrt_pd(a.data() ) );
}

}
}

#endif
