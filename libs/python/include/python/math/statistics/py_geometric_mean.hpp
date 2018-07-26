#pragma once

#include "math/linalg/matrix.hpp"
#include "math/statistics/geometric_mean.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type WrapperGeometricMean(A const &a)
{
  return GeometricMean(a);
}

inline void BuildGeometricMeanStatistics(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperGeometricMean<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperGeometricMean<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperGeometricMean<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperGeometricMean<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperGeometricMean<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperGeometricMean<ShapeLessArray<float>>);
};

}  // namespace statistics
}  // namespace math
}  // namespace fetch
