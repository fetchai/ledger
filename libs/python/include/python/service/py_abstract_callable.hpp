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

#include "service/abstract_callable.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {
namespace details {

template <typename T, typename arguments>
void BuildPacker(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Packer<T, arguments>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace details
};  // namespace service
};  // namespace fetch
namespace fetch {
namespace service {

void BuildAbstractCallable(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<AbstractCallable>(module, "AbstractCallable")
      .def(py::init<uint64_t>())
      .def("meta_data", &AbstractCallable::meta_data);
}
};  // namespace service
};  // namespace fetch
