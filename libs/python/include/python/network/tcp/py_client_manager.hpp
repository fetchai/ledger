#pragma once
#include "network/managementtcp.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace network
{

void BuildClientManager(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ClientManager>(module, "ClientManager" )
    .def(py::init< fetch::network::AbstractNetworkServer & >())
    .def("Broadcast", &ClientManager::Broadcast)
    .def("PushRequest", &ClientManager::PushRequest)
    .def("GetAddress", &ClientManager::GetAddress)
    .def("Leave", &ClientManager::Leave)
    .def("Send", &ClientManager::Send)
    .def("Join", &ClientManager::Join);

}
};
};

