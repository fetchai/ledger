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

#include "http/http_connection_manager.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPConnectionManager(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPConnectionManager>(module, "HTTPConnectionManager")
      .def(py::init<fetch::http::AbstractHTTPServer &>())
      .def("Leave", &HTTPConnectionManager::Leave)
      .def("PushRequest", &HTTPConnectionManager::PushRequest)
      .def("Join", &HTTPConnectionManager::Join)
      .def("GetAddress", &HTTPConnectionManager::GetAddress)
      .def("Send", &HTTPConnectionManager::Send);
}
};  // namespace http
};  // namespace fetch
