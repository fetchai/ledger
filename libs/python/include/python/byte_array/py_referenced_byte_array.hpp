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

#include "core/byte_array/byte_array.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace byte_array {

void BuildByteArray(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ByteArray, ConstByteArray>(module, "ByteArray")
      .def(py::init<>())
      .def(py::init<const char *>())
      .def(py::init<const std::string &>())
      .def(py::init<const fetch::byte_array::ByteArray &>())
      .def(py::init<std::initializer_list<ByteArray::value_type>>())
      .def(py::init<const fetch::byte_array::ByteArray &, const std::size_t &,
                    const std::size_t &>())
      .def(py::init<const fetch::byte_array::ByteArray::super_type &>())
      .def(py::init<const fetch::byte_array::ByteArray::super_type &, const std::size_t &,
                    const std::size_t &>())
      .def(py::self + fetch::byte_array::ByteArray())
      .def("Resize", &ByteArray::Resize)
      .def("operator[]",
           (fetch::byte_array::ConstByteArray::value_type & (ByteArray::*)(std::size_t)) &
               ByteArray::operator[])
      .def(
          "operator[]",
          (const fetch::byte_array::ConstByteArray::value_type &(ByteArray::*)(std::size_t) const) &
              ByteArray::operator[])
      .def("pointer",
           (const fetch::byte_array::ConstByteArray::value_type *(ByteArray::*)() const) &
               ByteArray::pointer)
      .def("pointer",
           (fetch::byte_array::ConstByteArray::value_type * (ByteArray::*)()) & ByteArray::pointer)
      .def("char_pointer", (const char *(ByteArray::*)() const) & ByteArray::char_pointer)
      .def("char_pointer", (char *(ByteArray::*)()) & ByteArray::char_pointer)
      .def("Reserve", &ByteArray::Reserve);
}

}  // namespace byte_array
}  // namespace fetch
