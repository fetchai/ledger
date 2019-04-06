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

#include "network/tcp_client.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace network {

void BuildTCPClient(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<TCPClient>(module, "TCPClient")
      .def(py::init<const std::string &, const std::string &,
                    fetch::network::TCPClient::network_manager_ptr_type>())
      .def(py::init<const std::string &, const uint16_t &,
                    fetch::network::TCPClient::network_manager_ptr_type>())
      .def("handle", &TCPClient::handle)
      .def("Send", &TCPClient::Send)
      .def("Address", &TCPClient::Address);
}
};  // namespace network
};  // namespace fetch
