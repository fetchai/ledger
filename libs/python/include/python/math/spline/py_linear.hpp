#ifndef LIBFETCHCORE_MATH_SPLINE_LINEAR_HPP
#define LIBFETCHCORE_MATH_SPLINE_LINEAR_HPP

#include "math/spline/linear.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace spline {

template <typename T>
void BuildSpline(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Spline<T>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */
      .def("operator()", &Spline<T>::operator())
      .def("size", &Spline<T>::size);
}
};  // namespace spline
};  // namespace math
};  // namespace fetch

#endif
