#ifndef LIBFETCHCORE_MEMORY_SHARED_HASHTABLE_HPP
#define LIBFETCHCORE_MEMORY_SHARED_HASHTABLE_HPP
#include "memory/shared_hashtable.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace memory
{

template< typename T >
void BuildSharedHashTable(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<SharedHashTable< T >, SharedArray<details::Record<T> >>(module, custom_name )
    .def(py::init< std::size_t >())
    .def(py::init<  >())
    .def(py::init< const SharedHashTable<T> & >())
    .def(py::init< SharedHashTable<T> && >())
    .def("Find", ( uint32_t (SharedHashTable< T >::*)(const byte_array::BasicByteArray &) ) &SharedHashTable< T >::Find)
    .def("Find", ( uint32_t (SharedHashTable< T >::*)(const byte_array::BasicByteArray &, uint32_t &) ) &SharedHashTable< T >::Find)
    .def("At", ( fetch::memory::SharedHashTable::type & (SharedHashTable< T >::*)(const uint32_t &) ) &SharedHashTable< T >::At)
    .def("At", ( const fetch::memory::SharedHashTable::type & (SharedHashTable< T >::*)(const uint32_t &) const ) &SharedHashTable< T >::At)
    .def("operator[]", ( fetch::memory::SharedHashTable::type & (SharedHashTable< T >::*)(const byte_array::BasicByteArray &) ) &SharedHashTable< T >::operator[])
    .def("operator[]", ( const fetch::memory::SharedHashTable::type & (SharedHashTable< T >::*)(const byte_array::BasicByteArray &) const ) &SharedHashTable< T >::operator[])
    .def("HasCapacityFor", &SharedHashTable< T >::HasCapacityFor);

}
};
};
namespace fetch
{
namespace memory
{
namespace details
{

template< typename T >
void BuildRecord(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Record< T >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}
};
};
};

#endif