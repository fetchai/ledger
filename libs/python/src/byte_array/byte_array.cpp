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

#include "core/byte_array/byte_array.hpp"
#include "python/byte_array/byte_array.hpp"

namespace fetch {
namespace byte_array {

void BuildByteArray(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ByteArray, ConstByteArray>(module, "ByteArray")
      .def(py::init<>())
      .def(py::init<char const *>())
      .def(py::init<std::string const &>())
      .def(py::init<fetch::byte_array::ByteArray const &>())
      .def(py::init<std::initializer_list<ByteArray::ValueType>>())
      .def(py::init<fetch::byte_array::ByteArray const &, std::size_t const &,
                    std::size_t const &>())
      .def(py::init<fetch::byte_array::ByteArray::SuperType const &>())
      .def(py::init<fetch::byte_array::ByteArray::SuperType const &, std::size_t const &,
                    std::size_t const &>())
      .def(py::self + fetch::byte_array::ByteArray())
      .def("Resize", &ByteArray::Resize)
      .def("Reserve", &ByteArray::Reserve);
}

}  // namespace byte_array
}  // namespace fetch
