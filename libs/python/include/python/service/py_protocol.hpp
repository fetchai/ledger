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

#include "service/protocol.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

void BuildProtocol(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Protocol>(module, "Protocol")
      .def(py::init<>()) /* No constructors found */
      .def("Expose", &Protocol::Expose)
      .def("Subscribe", &Protocol::Subscribe)
      .def("Unsubscribe", &Protocol::Unsubscribe)
      .def("operator[]", &Protocol::operator[])
      .def("feeds", &Protocol::feeds)
      .def("RegisterFeed", &Protocol::RegisterFeed);
}
};  // namespace service
};  // namespace fetch
