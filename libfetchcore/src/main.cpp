#include <pybind11/pybind11.h>

#include "byte_array/py_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/typed_byte_array_buffer.hpp"

#include "service/py_promise.hpp"
#include "service/py_client.hpp"
using namespace fetch;


namespace py = pybind11;

PYBIND11_MODULE(libfetchcore, m) {


  py::module byte_array_ns = m.def_submodule("byte_array");
  byte_array::Build( byte_array_ns );

  py::module service_ns = m.def_submodule("service");
  service::BuildPromise( service_ns );
  service::BuildClient( service_ns );
  
  py::module serializers_ns = m.def_submodule("serializers");
  py::class_<serializers::TypedByte_ArrayBuffer>(byte_array_ns, "TypedByteArrayBuffer")
    .def("Pack", &serializers::TypedByte_ArrayBuffer::Pack<int8_t>)

    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<uint8_t>)
    .def("Pack", &serializers::TypedByte_ArrayBuffer::Pack<uint16_t>)
    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<uint16_t>)            
    .def("Pack", &serializers::TypedByte_ArrayBuffer::Pack<uint32_t>)
    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<uint32_t>)
    .def("Pack", &serializers::TypedByte_ArrayBuffer::Pack<uint64_t>)
    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<uint64_t>)    
    
    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<int8_t>)
    .def("Pack", &serializers::TypedByte_ArrayBuffer::Pack<int16_t>)
    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<int16_t>)            
    .def("Pack", &serializers::TypedByte_ArrayBuffer::Pack<int32_t>)
    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<int32_t>)
    .def("Pack", &serializers::TypedByte_ArrayBuffer::Pack<int64_t>)
    .def("Unpack", &serializers::TypedByte_ArrayBuffer::Unpack<int64_t>)
    
    .def("Seek", &serializers::TypedByte_ArrayBuffer::Seek)
    .def("Tell", &serializers::TypedByte_ArrayBuffer::Tell)
    .def("data", &serializers::TypedByte_ArrayBuffer::data);
}
