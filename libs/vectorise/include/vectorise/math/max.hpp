#pragma once
namespace fetch {
namespace vectorize {

inline VectorRegister<float, 128> max(VectorRegister<float, 128> const &a,
                                      VectorRegister<float, 128> const &b)
{
  return VectorRegister<float, 128>(_mm_max_ps(a.data(), b.data()));
}

inline VectorRegister<double, 128> max(VectorRegister<double, 128> const &a,
                                       VectorRegister<double, 128> const &b)
{
  return VectorRegister<double, 128>(_mm_max_pd(a.data(), b.data()));
}

}  // namespace vectorize
}  // namespace fetch
