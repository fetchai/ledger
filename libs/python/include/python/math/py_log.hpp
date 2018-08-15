#pragma once

#include "math/linalg/matrix.hpp"
#include "math/log.hpp"
#include "math/ndarray.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <typename A>
inline A WrapperLog(A const &a)
{
  return Log(a);
}

inline void BuildLogStatistics(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperLog<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperLog<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperLog<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperLog<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperLog<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperLog<ShapeLessArray<float>>)
      .def(custom_name.c_str(), &WrapperLog<NDArray<double>>)
      .def(custom_name.c_str(), &WrapperLog<NDArray<float>>);
};

}  // namespace math
}  // namespace fetch
