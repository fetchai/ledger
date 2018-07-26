#pragma once

#include "math/linalg/matrix.hpp"
#include "math/statistics/mean.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type WrapperMean(A const &a)
{
  return Mean(a);
}

inline void BuildMeanStatistics(std::string const &custom_name,
                                pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperMean<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperMean<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperMean<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperMean<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperMean<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperMean<ShapeLessArray<float>>);
};

}  // namespace statistics
}  // namespace math
}  // namespace fetch
