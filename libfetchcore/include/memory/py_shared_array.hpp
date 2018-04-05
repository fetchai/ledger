#ifndef LIBFETCHCORE_MEMORY_SHARED_ARRAY_HPP
#define LIBFETCHCORE_MEMORY_SHARED_ARRAY_HPP
#include "memory/shared_array.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace memory
{

template< typename T >
void BuildSharedArray(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<SharedArray< T >>(module, custom_name )
    .def(py::init< const std::size_t & >())
    .def(py::init<  >())
    .def(py::init< const SharedArray<T> & >())
    .def(py::init< SharedArray<T> && >())
    .def("simd_size", &SharedArray< T >::simd_size)
    .def("begin", &SharedArray< T >::begin)
    .def("Set", &SharedArray< T >::Set)
    .def("end", &SharedArray< T >::end)
    .def("rend", &SharedArray< T >::rend)
    .def("rbegin", &SharedArray< T >::rbegin)
    .def("padded_size", &SharedArray< T >::padded_size)
    .def("At", ( T & (SharedArray< T >::*)(const std::size_t &) ) &SharedArray< T >::At)
    .def("At", ( const T & (SharedArray< T >::*)(const std::size_t &) const ) &SharedArray< T >::At)
    .def("operator[]", ( T & (SharedArray< T >::*)(const std::size_t &) ) &SharedArray< T >::operator[])
    .def("operator[]", ( const T & (SharedArray< T >::*)(const std::size_t &) const ) &SharedArray< T >::operator[])
    .def(py::self = py::self )
    .def("Copy", &SharedArray< T >::Copy)
    .def("pointer", &SharedArray< T >::pointer)
    .def("size", &SharedArray< T >::size);

}
};
};

#endif