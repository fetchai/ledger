#pragma once

#include "math/linalg/matrix.hpp"
#include "math/statistics/min.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type WrapperMin(A const &a)
{
  return Min(a);
}

inline void BuildMinStatistics(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperMin<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperMin<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperMin<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperMin<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperMin<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperMin<ShapeLessArray<float>>)
      .def(custom_name.c_str(), &WrapperMin<NDArray<double>>)
      .def(custom_name.c_str(), &WrapperMin<NDArray<float>>);
};

}  // namespace statistics
}  // namespace math
}  // namespace fetch
