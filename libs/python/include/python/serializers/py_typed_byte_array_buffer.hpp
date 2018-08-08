#pragma once
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
      .def("Seek", &TypedByteArrayBuffer::Seek)
      .def("Tell", &TypedByteArrayBuffer::Tell)
      .def("Reserve", &TypedByteArrayBuffer::Reserve);
}
};  // namespace serializers
};  // namespace fetch
