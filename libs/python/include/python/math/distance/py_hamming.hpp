#pragma once

#include "math/distance/hamming.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline typename A::type WrapperHamming(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return Hamming(a, b);
}

inline void BuildHammingDistance(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperHamming<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperHamming<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperHamming<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperHamming<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperHamming<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperHamming<ShapeLessArray<float>>);
};

}  // namespace distance
}  // namespace math
}  // namespace fetch
