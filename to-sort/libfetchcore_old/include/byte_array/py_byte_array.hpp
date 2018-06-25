#ifndef BYTE_ARRAY_PY_BYTE_ARRAY_HPP
#define BYTE_ARRAY_PY_BYTE_ARRAY_HPP
#include "byte_array/referenced_byte_array.hpp"
namespace fetch {
namespace byte_array {
void Build(pybind11::module &m)
{
  namespace py = pybind11;
  
  py::class_<ByteArray>(m, "ByteArray")
    .def(py::init<>())
    .def(py::init<std::string const &>())
    .def(py::init<ByteArray const &>())
    
    .def("Resize", &ByteArray::Resize)
    .def("Reserve", &ByteArray::Reserve)
    .def("capacity", &ByteArray::capacity)
    .def("SubArray", &ByteArray::SubArray)
    //    .def("Match", &byte_array::ByteArray::Match)
    .def("Find", &ByteArray::Find)
    .def("size", &ByteArray::size) ;


}
};
};
#endif
