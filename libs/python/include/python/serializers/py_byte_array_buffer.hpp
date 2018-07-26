#ifndef LIBFETCHCORE_SERIALIZER_BYTE_ARRAY_BUFFER_HPP
#define LIBFETCHCORE_SERIALIZER_BYTE_ARRAY_BUFFER_HPP
#include "serializers/byte_array_buffer.hpp"

#include "fetch_pybind.hpp"

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
      .def("Seek", &ByteArrayBuffer::Seek)
      .def("Tell", &ByteArrayBuffer::Tell)
      .def("Reserve", &ByteArrayBuffer::Reserve);
}
};  // namespace serializers
};  // namespace fetch

#endif
