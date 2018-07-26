#pragma once
namespace fetch {
namespace vectorize {

inline VectorRegister<float, 128> approx_reciprocal(VectorRegister<float, 128> const &x) {

  return VectorRegister<float, 128>( _mm_rcp_ps(x.data()) );
}
  

inline VectorRegister<double, 128> approx_reciprocal(VectorRegister<double, 128> const &x) {
  // TODO: Test this function
  return VectorRegister<double, 128>( _mm_cvtps_pd(_mm_rcp_ps(_mm_cvtpd_ps(x.data()))) );
}


}
}

