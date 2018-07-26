#pragma once

#include "math/distance/braycurtis.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline typename A::type WrapperBraycurtis(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return Braycurtis(a, b);
}

inline void BuildBraycurtisDistance(std::string const &custom_name,
                                    pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperBraycurtis<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperBraycurtis<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperBraycurtis<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperBraycurtis<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperBraycurtis<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperBraycurtis<ShapeLessArray<float>>);
};

}  // namespace distance
}  // namespace math
}  // namespace fetch
