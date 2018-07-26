#pragma once
#include "network/tcp_server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace network {

void BuildTCPServer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<TCPServer, fetch::network::AbstractNetworkServer>(module,
                                                               "TCPServer")
      .def(py::init<
           const uint16_t &,
           const fetch::network::TCPServer::network_manager_ptr_type &>())
      .def("GetAddress", &TCPServer::GetAddress)
      .def("PushRequest", &TCPServer::PushRequest)
      .def("Top", &TCPServer::Top)
      .def("Pop", &TCPServer::Pop)
      .def("Broadcast", &TCPServer::Broadcast)
      .def("Send", &TCPServer::Send)
      .def("has_requests", &TCPServer::has_requests);
}
};  // namespace network
};  // namespace fetch

