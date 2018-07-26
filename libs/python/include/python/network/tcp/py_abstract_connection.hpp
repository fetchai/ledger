#pragma once
#include "network/management/abstract_connection.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace network
{

void BuildAbstractClientConnection(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractClientConnection>(module, "AbstractClientConnection" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

