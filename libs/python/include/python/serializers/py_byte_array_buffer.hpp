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

#include "core/serializers/byte_array_buffer.hpp"
#include "pybind11/pybind11.h"

namespace fetch {
namespace serializers {

void BuildByteArrayBuffer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ByteArrayBuffer>(module, "ByteArrayBuffer")
      .def(py::init<>())
      .def(py::init<byte_array::ByteArray>())
      .def("WriteBytes", &ByteArrayBuffer::WriteBytes)
      .def("bytes_left", &ByteArrayBuffer::bytes_left)
      .def("data", &ByteArrayBuffer::data)
      .def("SkipBytes", &ByteArrayBuffer::SkipBytes)
      .def("ReadByteArray", &ByteArrayBuffer::ReadByteArray)
      .def("ReadBytes", &ByteArrayBuffer::ReadBytes)
      .def("Allocate", &ByteArrayBuffer::Allocate)
      .def("size", &ByteArrayBuffer::size)
      .def("seek", &ByteArrayBuffer::seek)
      .def("tell", &ByteArrayBuffer::tell)
      .def("Reserve", &ByteArrayBuffer::Reserve);
}
};  // namespace serializers
};  // namespace fetch
