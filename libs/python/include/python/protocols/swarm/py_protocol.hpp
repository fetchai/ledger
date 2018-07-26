#pragma once
#include "protocols/swarm/protocol.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace protocols {

void BuildSwarmProtocol(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<SwarmProtocol, fetch::protocols::SwarmController,
             fetch::service::Protocol, fetch::http::HTTPModule>(module,
                                                                "SwarmProtocol")
      .def(py::init<network::NetworkManager *, const uint64_t &,
                    fetch::protocols::SharedNodeDetails &>());
}
};  // namespace protocols
};  // namespace fetch
