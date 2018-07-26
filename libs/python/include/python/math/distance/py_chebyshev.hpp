#pragma once

#include "math/distance/chebyshev.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline typename A::type WrapperChebyshev(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return Chebyshev(a, b);
}

inline void BuildChebyshevDistance(std::string const &custom_name,
                                   pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperChebyshev<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperChebyshev<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperChebyshev<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperChebyshev<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperChebyshev<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperChebyshev<ShapeLessArray<float>>);
};

}  // namespace distance
}  // namespace math
}  // namespace fetch
