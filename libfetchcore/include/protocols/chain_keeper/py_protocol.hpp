#ifndef LIBFETCHCORE_PROTOCOLS_CHAIN_KEEPER_PROTOCOL_HPP
#define LIBFETCHCORE_PROTOCOLS_CHAIN_KEEPER_PROTOCOL_HPP
#include "protocols/chain_keeper/protocol.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace protocols
{

void BuildChainKeeperProtocol(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ChainKeeperProtocol, fetch::protocols::ChainKeeperController, fetch::service::Protocol, fetch::http::HTTPModule>(module, "ChainKeeperProtocol" )
    .def(py::init< network::ThreadManager *, const uint64_t &, fetch::protocols::EntryPoint & >())
    .def("Ping", &ChainKeeperProtocol::Ping);

}
};
};

#endif