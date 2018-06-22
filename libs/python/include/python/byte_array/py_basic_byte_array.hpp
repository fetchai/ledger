#ifndef LIBFETCHCORE_BYTE_ARRAY_BASIC_BYTE_ARRAY_HPP
#define LIBFETCHCORE_BYTE_ARRAY_BASIC_BYTE_ARRAY_HPP
#include "byte_array/basic_byte_array.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace byte_array
{


void BuildBasicByteArray(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<BasicByteArray>(module, "BasicByteArray" )
    .def(py::init<  >())
    .def(py::init< const std::size_t & >())
    .def(py::init< const char * >())
    .def(py::init< std::initializer_list<BasicByteArray::container_type> >())
    .def(py::init< const std::string & >())
    .def(py::init< const fetch::byte_array::BasicByteArray::self_type & >())
    .def(py::init< const fetch::byte_array::BasicByteArray::self_type &, const std::size_t &, const std::size_t & >())
    .def("AsInt", &BasicByteArray::AsInt)
    .def(py::self != fetch::byte_array::BasicByteArray::self_type() )
    //    .def(py::self != char*() )
    .def(py::self < fetch::byte_array::BasicByteArray::self_type() )
    .def(py::self > fetch::byte_array::BasicByteArray::self_type() )
    .def("AsFloat", &BasicByteArray::AsFloat)
    .def(py::self == fetch::byte_array::BasicByteArray::self_type() )
    //    .def(py::self == char*() )
    .def(py::self + fetch::byte_array::BasicByteArray::self_type() )
    //    .def("char_pointer", &BasicByteArray::char_pointer)
    //    .def("operator[]", &BasicByteArray::operator[])
    .def("SubArray", &BasicByteArray::SubArray)
    .def("capacity", &BasicByteArray::capacity)
    .def("Copy", &BasicByteArray::Copy)
    //    .def("pointer", &BasicByteArray::pointer)
    .def("Find", &BasicByteArray::Find)
    .def("Match", ( bool (BasicByteArray::*)(const fetch::byte_array::BasicByteArray::self_type &, std::size_t) const ) &BasicByteArray::Match)
    .def("Match", ( bool (BasicByteArray::*)(const fetch::byte_array::BasicByteArray::container_type *, std::size_t) const ) &BasicByteArray::Match)
    .def("size", &BasicByteArray::size);

}
};
};

#endif
