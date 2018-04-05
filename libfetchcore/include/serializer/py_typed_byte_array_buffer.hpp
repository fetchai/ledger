#ifndef LIBFETCHCORE_SERIALIZER_TYPED_BYTE_ARRAY_BUFFER_HPP
#define LIBFETCHCORE_SERIALIZER_TYPED_BYTE_ARRAY_BUFFER_HPP
#include "serializer/typed_byte_array_buffer.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace serializers
{

void BuildTypedByte_ArrayBuffer(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<TypedByte_ArrayBuffer>(module, "TypedByte_ArrayBuffer" )
    .def(py::init<  >())
    .def(py::init< fetch::serializers::TypedByte_ArrayBuffer::byte_array_type >())
    .def("WriteBytes", &TypedByte_ArrayBuffer::WriteBytes)
    .def("bytes_left", &TypedByte_ArrayBuffer::bytes_left)
    .def("data", &TypedByte_ArrayBuffer::data)
    .def("SkipBytes", &TypedByte_ArrayBuffer::SkipBytes)
    .def("ReadByteArray", &TypedByte_ArrayBuffer::ReadByteArray)
    .def("ReadBytes", &TypedByte_ArrayBuffer::ReadBytes)
    .def("Allocate", &TypedByte_ArrayBuffer::Allocate)
    .def("size", &TypedByte_ArrayBuffer::size)
    .def("Seek", &TypedByte_ArrayBuffer::Seek)
    .def("Tell", &TypedByte_ArrayBuffer::Tell)
    .def("Reserve", &TypedByte_ArrayBuffer::Reserve);

}
};
};

#endif