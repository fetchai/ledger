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

#include "service/service_client.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

template <typename T>
void BuildServiceClient(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ServiceClient<T>, T, fetch::service::ServiceClientInterface,
             fetch::service::ServiceServerInterface>(module, custom_name)
      .def(py::init<const std::string &, const uint16_t &,
                    typename fetch::service::ServiceClient<T>::network_manager_ptr_type>())
      .def("PushMessage", &ServiceClient<T>::PushMessage)
      .def("ConnectionFailed", &ServiceClient<T>::ConnectionFailed);
}
};  // namespace service
};  // namespace fetch
