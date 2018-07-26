#pragma once
namespace fetch {
namespace vectorize {

inline VectorRegister<float, 128> min( VectorRegister<float, 128> const &a,
                                               VectorRegister<float, 128> const &b) {
  return VectorRegister<float, 128>( _mm_min_ps(a.data(), b.data() ) );
}

inline VectorRegister<double, 128> min( VectorRegister<double, 128> const &a,
                                                 VectorRegister<double, 128> const &b) {
  return VectorRegister<double, 128>( _mm_min_pd(a.data(), b.data() ) );
}


}
}

