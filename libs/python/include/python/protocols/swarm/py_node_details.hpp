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

#include "protocols/swarm/node_details.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace protocols {

void BuildSharedNodeDetails(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<SharedNodeDetails>(module, "SharedNodeDetails")
      .def(py::init<>())
      .def(py::init<const fetch::protocols::SharedNodeDetails &>())
      .def("with_details", &SharedNodeDetails::with_details)
      .def("AddEntryPoint", &SharedNodeDetails::AddEntryPoint)
      .def(py::self == fetch::protocols::SharedNodeDetails())
      .def("details", &SharedNodeDetails::details)
      .def("default_port", &SharedNodeDetails::default_port)
      .def("default_http_port", &SharedNodeDetails::default_http_port);
}
};  // namespace protocols
};  // namespace fetch
