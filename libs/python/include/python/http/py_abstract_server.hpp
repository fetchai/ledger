#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "http/abstract_server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildAbstractHTTPServer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<AbstractHTTPServer>(module, "AbstractHTTPServer")
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace http
};  // namespace fetch
