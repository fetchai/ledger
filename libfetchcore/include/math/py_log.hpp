#ifndef LIBFETCHCORE_MATH_LOG_HPP
#define LIBFETCHCORE_MATH_LOG_HPP
#include "math/log.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace math
{

void BuildLog(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Log>(module, "Log" )
    .def(py::init<>()) /* No constructors found */
    .def("operator()", &Log::operator());

}
};
};

#endif