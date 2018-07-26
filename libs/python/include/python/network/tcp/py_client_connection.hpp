#pragma once
#include "network/tcp/client_connection.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace network {

void BuildClientConnection(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ClientConnection, fetch::network::AbstractClientConnection>(module, "ClientConnection")
      .def(py::init<asio::ip::tcp::tcp::socket, fetch::network::ClientManager &>())
      .def("Start", &ClientConnection::Start)
      .def("handle", &ClientConnection::handle)
      .def("Send", &ClientConnection::Send)
      .def("Address", &ClientConnection::Address);
}
};  // namespace network
};  // namespace fetch
