#pragma once
#include "network/tcp/abstract_server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace network {

void BuildAbstractNetworkServer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<AbstractNetworkServer>(module, "AbstractNetworkServer")
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace network
};  // namespace fetch
