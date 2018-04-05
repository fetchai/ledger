#ifndef LIBFETCHCORE_NETWORK_TCP_SERVER_HPP
#define LIBFETCHCORE_NETWORK_TCP_SERVER_HPP
#include "network/tcp_server.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace network
{

void BuildTCPServer(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<TCPServer, fetch::network::AbstractNetworkServer>(module, "TCPServer" )
    .def(py::init< const uint16_t &, const fetch::network::TCPServer::thread_manager_ptr_type & >())
    .def("GetAddress", &TCPServer::GetAddress)
    .def("PushRequest", &TCPServer::PushRequest)
    .def("Top", &TCPServer::Top)
    .def("Pop", &TCPServer::Pop)
    .def("Broadcast", &TCPServer::Broadcast)
    .def("Send", &TCPServer::Send)
    .def("has_requests", &TCPServer::has_requests);

}
};
};

#endif