#pragma once
#include "http/connection.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPConnection(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPConnection, fetch::http::AbstractHTTPConnection>(
      module, "HTTPConnection")
      .def(py::init<asio::ip::tcp::tcp::socket,
                    fetch::http::HTTPConnectionManager &>())
      .def("socket", &HTTPConnection::socket)
      .def("ReadHeader", &HTTPConnection::ReadHeader)
      .def("ReadBody", &HTTPConnection::ReadBody)
      .def("HandleError", &HTTPConnection::HandleError)
      .def("Send", &HTTPConnection::Send)
      .def("Write", &HTTPConnection::Write)
      .def("Start", &HTTPConnection::Start)
      .def("Address", &HTTPConnection::Address)
      .def("Close", &HTTPConnection::Close);
}
};  // namespace http
};  // namespace fetch

