#ifndef LIBFETCHCORE_NETWORK_TCP_ABSTRACT_CONNECTION_HPP
#define LIBFETCHCORE_NETWORK_TCP_ABSTRACT_CONNECTION_HPP
#include "network/management/abstract_connection.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace network {

void BuildAbstractClientConnection(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<AbstractClientConnection>(module, "AbstractClientConnection")
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace network
};  // namespace fetch

#endif
