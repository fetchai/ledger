#ifndef LIBFETCHCORE_NETWORK_TCP_ABSTRACT_SERVER_HPP
#define LIBFETCHCORE_NETWORK_TCP_ABSTRACT_SERVER_HPP
#include "network/tcp/abstract_server.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace network
{

void BuildAbstractNetworkServer(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractNetworkServer>(module, "AbstractNetworkServer" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif
