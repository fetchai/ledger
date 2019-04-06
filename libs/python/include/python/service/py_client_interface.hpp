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

#include "service/client_interface.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

void BuildServiceClientInterface(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ServiceClientInterface>(module, "ServiceClientInterface")
      .def(py::init<>())
      .def("Subscribe", &ServiceClientInterface::Subscribe)
      .def("CallWithPackedArguments", &ServiceClientInterface::CallWithPackedArguments)
      .def("Unsubscribe", &ServiceClientInterface::Unsubscribe);
}
};  // namespace service
};  // namespace fetch
