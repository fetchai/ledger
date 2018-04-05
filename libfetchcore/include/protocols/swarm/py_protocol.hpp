#ifndef LIBFETCHCORE_PROTOCOLS_SWARM_PROTOCOL_HPP
#define LIBFETCHCORE_PROTOCOLS_SWARM_PROTOCOL_HPP
#include "protocols/swarm/protocol.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace protocols
{

void BuildSwarmProtocol(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<SwarmProtocol, fetch::protocols::SwarmController, fetch::service::Protocol, fetch::http::HTTPModule>(module, "SwarmProtocol" )
    .def(py::init< network::ThreadManager *, const uint64_t &, fetch::protocols::SharedNodeDetails & >());

}
};
};

#endif