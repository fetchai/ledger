#ifndef LIBFETCHCORE_STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#define LIBFETCHCORE_STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#include "storage/versioned_random_access_stack.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace storage
{

template< typename T, typename B >
void BuildVersionedRandomAccessStack(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<VersionedRandomAccessStack< T, B >, RandomAccessStack<T, B>>(module, custom_name )
    .def(py::init<>()) /* No constructors found */
    .def("Load", &VersionedRandomAccessStack< T, B >::Load)
    .def("Set", &VersionedRandomAccessStack< T, B >::Set)
    .def("Get", ( typename RandomAccessStack<T>::type (VersionedRandomAccessStack< T, B >::*)(const std::size_t &) const ) &VersionedRandomAccessStack< T, B >::Get)
    .def("Get", ( void (VersionedRandomAccessStack< T, B >::*)(const std::size_t &, typename fetch::storage::VersionedRandomAccessStack<T, B>::type &) const ) &VersionedRandomAccessStack< T, B >::Get)
    .def("ResetBookmark", &VersionedRandomAccessStack< T, B >::ResetBookmark)
    .def("Clear", &VersionedRandomAccessStack< T, B >::Clear)
    .def("Revert", &VersionedRandomAccessStack< T, B >::Revert)
    .def("Pop", &VersionedRandomAccessStack< T, B >::Pop)
    .def("NextBookmark", &VersionedRandomAccessStack< T, B >::NextBookmark)
    .def("Swap", &VersionedRandomAccessStack< T, B >::Swap)
    .def("Commit", &VersionedRandomAccessStack< T, B >::Commit)
    .def("Push", &VersionedRandomAccessStack< T, B >::Push)
    .def("New", &VersionedRandomAccessStack< T, B >::New)
    .def("PreviousBookmark", &VersionedRandomAccessStack< T, B >::PreviousBookmark)
    .def("Top", &VersionedRandomAccessStack< T, B >::Top)
    .def("empty", &VersionedRandomAccessStack< T, B >::empty)
    .def("size", &VersionedRandomAccessStack< T, B >::size);

}
};
};

#endif
