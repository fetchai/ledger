#ifndef LIBFETCHCORE_MATH_DISTANCE_EUCLIDEAN_HPP
#define LIBFETCHCORE_MATH_DISTANCE_EUCLIDEAN_HPP

#include "math/distance/euclidean.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline typename A::type WrapperEuclidean(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return Euclidean(a, b);
}

inline void BuildEuclideanDistance(std::string const &custom_name,
                                   pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperEuclidean<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperEuclidean<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperEuclidean<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperEuclidean<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperEuclidean<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperEuclidean<ShapeLessArray<float>>);
};

}  // namespace distance
}  // namespace math
}  // namespace fetch

#endif
