#pragma once
#include "http/abstract_server.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace http
{

void BuildAbstractHTTPServer(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractHTTPServer>(module, "AbstractHTTPServer" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

