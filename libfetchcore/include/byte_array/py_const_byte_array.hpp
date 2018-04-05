#ifndef LIBFETCHCORE_BYTE_ARRAY_CONST_BYTE_ARRAY_HPP
#define LIBFETCHCORE_BYTE_ARRAY_CONST_BYTE_ARRAY_HPP
#include "byte_array/const_byte_array.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace byte_array
{

void BuildConstByteArray(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ConstByteArray, fetch::byte_array::BasicByteArray>(module, "ConstByteArray" )
    .def(py::init<  >())
    .def(py::init< const char * >())
    .def(py::init< const std::string & >())
    .def(py::init< const fetch::byte_array::ConstByteArray & >())
    .def(py::init< std::initializer_list<container_type> >())
    .def(py::init< const fetch::byte_array::ConstByteArray &, const std::size_t &, const std::size_t & >())
    .def(py::init< const fetch::byte_array::ByteArray & >())
    .def(py::init< const fetch::byte_array::ByteArray &, const std::size_t &, const std::size_t & >())
    .def(py::init< const fetch::byte_array::ConstByteArray::super_type & >())
    .def(py::init< const fetch::byte_array::ConstByteArray::super_type &, const std::size_t &, const std::size_t & >());

}
};
};

#endif