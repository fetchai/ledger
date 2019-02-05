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

#include "fetch_pybind.hpp"
#include "storage/random_access_stack.hpp"

namespace fetch {
namespace storage {

template <typename T, typename D>
void BuildRandomAccessStack(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<RandomAccessStack<T, D>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */
      .def("Load", &RandomAccessStack<T, D>::Load)
      .def("header_extra", &RandomAccessStack<T, D>::header_extra)
      .def("Set", &RandomAccessStack<T, D>::Set)
      .def("Get", &RandomAccessStack<T, D>::Get)
      .def("Top", &RandomAccessStack<T, D>::Top)
      .def("Pop", &RandomAccessStack<T, D>::Pop)
      .def("SetExtraHeader", &RandomAccessStack<T, D>::SetExtraHeader)
      .def("Swap", &RandomAccessStack<T, D>::Swap)
      .def("Push", &RandomAccessStack<T, D>::Push)
      .def("New", &RandomAccessStack<T, D>::New)
      .def("Clear", &RandomAccessStack<T, D>::Clear)
      .def("empty", &RandomAccessStack<T, D>::empty)
      .def("size", &RandomAccessStack<T, D>::size);
}
};  // namespace storage
};  // namespace fetch
