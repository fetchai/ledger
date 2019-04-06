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

#include "service/server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

template <typename T>
void BuildServiceServer(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ServiceServer<T>, T, fetch::service::ServiceServerInterface>(module, custom_name)
      .def(
          py::init<uint16_t, typename fetch::service::ServiceServer<T>::network_manager_ptr_type>())
      .def("ServiceInterfaceOf", &ServiceServer<T>::ServiceInterfaceOf);
}
};  // namespace service
};  // namespace fetch
