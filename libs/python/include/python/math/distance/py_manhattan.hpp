#pragma once

#include "math/distance/manhattan.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline typename A::type WrapperManhattan(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return Manhattan(a, b);
}

inline void BuildManhattanDistance(std::string const &custom_name,
                                   pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperManhattan<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperManhattan<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperManhattan<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperManhattan<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperManhattan<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperManhattan<ShapeLessArray<float>>);
};

}  // namespace distance
}  // namespace math
}  // namespace fetch

