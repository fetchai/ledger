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

namespace fetch {
namespace storage {

template <typename S>
void BuildFileObjectImplementation(std::string const &custom_name, pybind11::module &module)
{
  /*
  namespace py = pybind11;
  py::class_<FileObjectImplementation< S >>(module, custom_name )
    .def(py::init< const uint64_t &,
  fetch::storage::FileObjectImplementation::stack_type & >()) .def("Read",
  &FileObjectImplementation< S >::Read) .def("tell", &FileObjectImplementation<
  S >::tell) .def("Write", &FileObjectImplementation< S >::Write)
    .def("file_position", &FileObjectImplementation< S >::file_position)
    .def("seek", &FileObjectImplementation< S >::seek)
    .def("Shrink", &FileObjectImplementation< S >::Shrink)
    .def("Size", &FileObjectImplementation< S >::Size);
  */
}
};  // namespace storage
};  // namespace fetch
