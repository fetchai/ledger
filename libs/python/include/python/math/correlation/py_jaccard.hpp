#pragma once

#include "math/correlation/jaccard.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace correlation {

template <typename A>
inline typename A::type WrapperJaccard(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return Jaccard(a, b);
}

inline void BuildJaccardCorrelation(std::string const &custom_name,
                                    pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperJaccard<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperJaccard<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperJaccard<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperJaccard<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperJaccard<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperJaccard<ShapeLessArray<float>>);
};

template <typename A>
inline typename A::type WrapperGeneralisedJaccard(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return GeneralisedJaccard(a, b);
}

inline void BuildGeneralisedJaccardCorrelation(std::string const &custom_name,
                                               pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperGeneralisedJaccard<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperGeneralisedJaccard<Matrix<float>>)
      .def(custom_name.c_str(),
           &WrapperGeneralisedJaccard<RectangularArray<double>>)
      .def(custom_name.c_str(),
           &WrapperGeneralisedJaccard<RectangularArray<float>>)
      .def(custom_name.c_str(),
           &WrapperGeneralisedJaccard<ShapeLessArray<double>>)
      .def(custom_name.c_str(),
           &WrapperGeneralisedJaccard<ShapeLessArray<float>>);
};

}  // namespace correlation
}  // namespace math
}  // namespace fetch
