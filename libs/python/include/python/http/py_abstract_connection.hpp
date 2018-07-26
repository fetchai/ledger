#pragma once
#include "http/abstract_connection.hpp"

#include"fetch_pybind.hpp"

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

