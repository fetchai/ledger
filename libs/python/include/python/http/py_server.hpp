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

#include "http/server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPServer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPServer, fetch::http::AbstractHTTPServer>(module, "HTTPServer")
      .def(py::init<const uint16_t &, const fetch::http::HTTPServer::network_manager_ptr_type &>())
      .def("AddMiddleware",
           (void (HTTPServer::*)(const fetch::http::HTTPServer::request_middleware_type &)) &
               HTTPServer::AddMiddleware)
      .def("AddMiddleware",
           (void (HTTPServer::*)(const fetch::http::HTTPServer::response_middleware_type &)) &
               HTTPServer::AddMiddleware)
      .def("AddModule", &HTTPServer::AddModule)
      .def("PushRequest", &HTTPServer::PushRequest)
      .def("AddView", &HTTPServer::AddView)
      .def("Accept", &HTTPServer::Accept);
}
};  // namespace http
};  // namespace fetch
