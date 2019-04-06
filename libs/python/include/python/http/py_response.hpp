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

#include "http/response.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPResponse(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPResponse>(module, "HTTPResponse")
      .def(py::init<byte_array::ByteArray, const fetch::http::MimeType &,
                    const fetch::http::Status &>())
      .def("body", &HTTPResponse::body)
      .def("status", (const fetch::http::Status &(HTTPResponse::*)() const) & HTTPResponse::status)
      .def("status", (fetch::http::Status & (HTTPResponse::*)()) & HTTPResponse::status)
      .def("mime_type",
           (const fetch::http::MimeType &(HTTPResponse::*)() const) & HTTPResponse::mime_type)
      .def("mime_type", (fetch::http::MimeType & (HTTPResponse::*)()) & HTTPResponse::mime_type)
      .def("header", (const fetch::http::Header &(HTTPResponse::*)() const) & HTTPResponse::header)
      .def("header", (fetch::http::Header & (HTTPResponse::*)()) & HTTPResponse::header);
}
};  // namespace http
};  // namespace fetch
