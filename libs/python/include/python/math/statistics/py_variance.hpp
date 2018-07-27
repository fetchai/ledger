#pragma once

#include "math/linalg/matrix.hpp"
#include "math/statistics/variance.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type WrapperVariance(A const &a)
{
  return Variance(a);
}

inline void BuildVarianceStatistics(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperVariance<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperVariance<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperVariance<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperVariance<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperVariance<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperVariance<ShapeLessArray<float>>);
};

}  // namespace statistics
}  // namespace math
}  // namespace fetch
