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

#include "storage/variant_stack.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace storage {

void BuildVariantStack(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<VariantStack>(module, "VariantStack")
      .def(py::init<>()) /* No constructors found */
      .def("Load", &VariantStack::Load)
      .def("Clear", &VariantStack::Clear)
      .def("Pop", &VariantStack::Pop)
      .def("New", &VariantStack::New)
      .def("Type", &VariantStack::Type)
      .def("empty", &VariantStack::empty)
      .def("size", &VariantStack::size);
}
};  // namespace storage
};  // namespace fetch
