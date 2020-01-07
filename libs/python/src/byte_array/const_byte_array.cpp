//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "python/byte_array/const_byte_array.hpp"

namespace fetch {
namespace byte_array {

void BuildConstByteArray(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ConstByteArray>(module, "ConstByteArray")
      .def(py::init<>())
      .def(py::init<std::size_t const &>())
      .def(py::init<char const *>())
      .def(py::init<unsigned char const *const, std::size_t>())
      .def(py::init<std::initializer_list<ConstByteArray::ValueType>>())
      .def(py::init<std::string const &>())
      .def(py::init<fetch::byte_array::ConstByteArray::SelfType const &>())
      .def(py::init<fetch::byte_array::ConstByteArray::SelfType const &, std::size_t const &,
                    std::size_t const &>())
      .def("AsInt", &ConstByteArray::AsInt)
      .def(py::self != fetch::byte_array::ConstByteArray::SelfType())
      .def(py::self < fetch::byte_array::ConstByteArray::SelfType())
      .def(py::self > fetch::byte_array::ConstByteArray::SelfType())
      .def("AsFloat", &ConstByteArray::AsFloat)
      .def(py::self == fetch::byte_array::ConstByteArray::SelfType())
      .def(py::self + fetch::byte_array::ConstByteArray::SelfType())
      .def("SubArray",
           static_cast<ConstByteArray (ConstByteArray::*)(std::size_t, std::size_t) const>(
               &ConstByteArray::SubArray))
      .def("capacity", &ConstByteArray::capacity)
      .def("Copy", &ConstByteArray::Copy)
      .def("Find", &ConstByteArray::Find)
      .def("Match",
           static_cast<bool (ConstByteArray::*)(fetch::byte_array::ConstByteArray::SelfType const &,
                                                std::size_t) const>(&ConstByteArray::Match))
      .def("Match", static_cast<bool (ConstByteArray::*)(
                        fetch::byte_array::ConstByteArray::ValueType const *, std::size_t) const>(
                        &ConstByteArray::Match))
      .def("size", &ConstByteArray::size)
      .def("AsBytes",
           [](ConstByteArray const &a) { return py::bytes(a.char_pointer(), a.size()); });
}

}  // namespace byte_array
}  // namespace fetch
