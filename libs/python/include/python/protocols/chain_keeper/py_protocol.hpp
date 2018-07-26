#pragma once
#include "protocols/chain_keeper/protocol.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace protocols
{

void BuildChainKeeperProtocol(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ChainKeeperProtocol, fetch::protocols::ChainKeeperController, fetch::service::Protocol, fetch::http::HTTPModule>(module, "ChainKeeperProtocol" )
    .def(py::init< network::NetworkManager *, const uint64_t &, fetch::protocols::EntryPoint & >())
    .def("Ping", &ChainKeeperProtocol::Ping);

}
};
};

