#ifndef LIBFETCHCORE_NETWORK_TCP_ABSTRACT_CONNECTION_HPP
#define LIBFETCHCORE_NETWORK_TCP_ABSTRACT_CONNECTION_HPP
#include "network/tcp/abstract_connection.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
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

#endif