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

#include "serializers/typed_byte_array_buffer.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace serializers {

void BuildTypedByteArrayBuffer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<TypedByteArrayBuffer>(module, "TypedByteArrayBuffer")
      .def(py::init<>())
      .def(py::init<fetch::serializers::TypedByteArrayBuffer::byte_array_type>())
      .def("WriteBytes", &TypedByteArrayBuffer::WriteBytes)
      .def("bytes_left", &TypedByteArrayBuffer::bytes_left)
      .def("data", &TypedByteArrayBuffer::data)
      .def("SkipBytes", &TypedByteArrayBuffer::SkipBytes)
      .def("ReadByteArray", &TypedByteArrayBuffer::ReadByteArray)
      .def("ReadBytes", &TypedByteArrayBuffer::ReadBytes)
      .def("Allocate", &TypedByteArrayBuffer::Allocate)
      .def("size", &TypedByteArrayBuffer::size)
      .def("seek", &TypedByteArrayBuffer::seek)
      .def("tell", &TypedByteArrayBuffer::tell)
      .def("Reserve", &TypedByteArrayBuffer::Reserve);
}
};  // namespace serializers
};  // namespace fetch
