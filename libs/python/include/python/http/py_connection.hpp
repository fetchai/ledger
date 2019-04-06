#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "http/connection.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPConnection(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPConnection, fetch::http::AbstractHTTPConnection>(module, "HTTPConnection")
      .def(py::init<asio::ip::tcp::tcp::socket, fetch::http::HTTPConnectionManager &>())
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
