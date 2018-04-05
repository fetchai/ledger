#ifndef LIBFETCHCORE_NETWORK_TCP_CLIENT_HPP
#define LIBFETCHCORE_NETWORK_TCP_CLIENT_HPP
#include "network/tcp_client.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace network
{

void BuildTCPClient(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<TCPClient>(module, "TCPClient" )
    .def(py::init< const std::string &, const std::string &, fetch::network::TCPClient::thread_manager_ptr_type >())
    .def(py::init< const std::string &, const uint16_t &, fetch::network::TCPClient::thread_manager_ptr_type >())
    .def("handle", &TCPClient::handle)
    .def("Send", &TCPClient::Send)
    .def("Address", &TCPClient::Address);

}
};
};

#endif