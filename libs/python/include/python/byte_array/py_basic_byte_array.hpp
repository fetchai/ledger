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

#include "core/byte_array/const_byte_array.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace byte_array {

fetch::byte_array::ConstByteArray BytesToFetchBytes(py::bytes const &b)
{
  std::string s = std::string(b);
  return fetch::byte_array::ConstByteArray((const unsigned char *const)s.c_str(), s.size());
}

void BuildConstByteArray(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ConstByteArray>(module, "ConstByteArray")
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def(py::init<const char *>())
      .def(py::init<const unsigned char *const, std::size_t>())
      .def(py::init<std::initializer_list<ConstByteArray::container_type>>())
      .def(py::init<const std::string &>())
      .def(py::init<const fetch::byte_array::ConstByteArray::self_type &>())
      .def(py::init<const fetch::byte_array::ConstByteArray::self_type &, const std::size_t &,
                    const std::size_t &>())
      .def("AsInt", &ConstByteArray::AsInt)
      .def(py::self != fetch::byte_array::ConstByteArray::self_type())
      .def(py::self < fetch::byte_array::ConstByteArray::self_type())
      .def(py::self > fetch::byte_array::ConstByteArray::self_type())
      .def("AsFloat", &ConstByteArray::AsFloat)
      .def(py::self == fetch::byte_array::ConstByteArray::self_type())
      .def(py::self + fetch::byte_array::ConstByteArray::self_type())
      .def("SubArray",
           static_cast<ConstByteArray (ConstByteArray::*)(std::size_t const &, std::size_t) const>(
               &ConstByteArray::SubArray))
      .def("capacity", &ConstByteArray::capacity)
      .def("Copy", &ConstByteArray::Copy)
      .def("Find", &ConstByteArray::Find)
      .def("Match", (bool (ConstByteArray::*)(const fetch::byte_array::ConstByteArray::self_type &,
                                              std::size_t) const) &
                        ConstByteArray::Match)
      .def("Match",
           (bool (ConstByteArray::*)(const fetch::byte_array::ConstByteArray::container_type *,
                                     std::size_t) const) &
               ConstByteArray::Match)
      .def("size", &ConstByteArray::size)
      .def("AsBytes",
           [](ConstByteArray const &a) { return py::bytes(a.char_pointer(), a.size()); });

  module.def("BytesToFetchBytes", &BytesToFetchBytes, "Convertion utility");
}

}  // namespace byte_array
}  // namespace fetch
