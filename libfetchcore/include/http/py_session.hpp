#ifndef LIBFETCHCORE_HTTP_SESSION_HPP
#define LIBFETCHCORE_HTTP_SESSION_HPP
#include "http/session.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace http
{

void BuildSession(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Session>(module, "Session" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif