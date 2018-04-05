#ifndef LIBFETCHCORE_HTTP_ABSTRACT_CONNECTION_HPP
#define LIBFETCHCORE_HTTP_ABSTRACT_CONNECTION_HPP
#include "http/abstract_connection.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace http
{

void BuildAbstractHTTPConnection(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractHTTPConnection>(module, "AbstractHTTPConnection" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif