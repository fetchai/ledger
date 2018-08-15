#pragma once

#include "math/exp.hpp"
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <typename A>
inline A WrapperExp(A const &a)
{
  return Exp(a);
}

inline void BuildExpStatistics(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperExp<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperExp<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperExp<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperExp<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperExp<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperExp<ShapeLessArray<float>>)
      .def(custom_name.c_str(), &WrapperExp<NDArray<double>>)
      .def(custom_name.c_str(), &WrapperExp<NDArray<float>>);
};

}  // namespace math
}  // namespace fetch
