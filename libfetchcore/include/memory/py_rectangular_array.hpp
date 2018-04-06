#ifndef LIBFETCHCORE_MEMORY_RECTANGULAR_ARRAY_HPP
#define LIBFETCHCORE_MEMORY_RECTANGULAR_ARRAY_HPP
#include "memory/rectangular_array.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace memory
{

template< typename T >
void BuildRectangularArray(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<RectangularArray< T >>(module, custom_name )
    .def(py::init<  >())
    .def(py::init< const std::size_t & >())
    .def(py::init< const std::size_t &, const std::size_t & >())
    .def(py::init< RectangularArray<T> && >())
    .def(py::init< const RectangularArray<T> & >())
    //    .def(py::self = py::self )
    .def("rend", &RectangularArray< T >::rend)
    .def("Reshape", &RectangularArray< T >::Reshape)
    .def("height", &RectangularArray< T >::height)
    .def("Save", &RectangularArray< T >::Save)
    .def("size", &RectangularArray< T >::size)
    .def("end", &RectangularArray< T >::end)
    .def("width", &RectangularArray< T >::width)
    .def("Copy", &RectangularArray< T >::Copy)
    .def("Resize", ( void (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &) ) &RectangularArray< T >::Resize)
    .def("Resize", ( void (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &, const typename fetch::memory::RectangularArray< T >::size_type &) ) &RectangularArray< T >::Resize)
    .def("Flatten", &RectangularArray< T >::Flatten)
    .def("begin", &RectangularArray< T >::begin)
    .def("Rotate", ( void (RectangularArray< T >::*)(const double &, const typename fetch::memory::RectangularArray< T >::type) ) &RectangularArray< T >::Rotate)
    .def("Rotate", ( void (RectangularArray< T >::*)(const double &, const double &, const double &, const typename fetch::memory::RectangularArray< T >::type) ) &RectangularArray< T >::Rotate)
    .def(py::self != py::self )
    .def(py::self == py::self )
    .def("operator[]", ( const typename fetch::memory::RectangularArray< T >::type & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &) const ) &RectangularArray< T >::operator[])
    .def("operator[]", ( typename fetch::memory::RectangularArray< T >::type & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &) ) &RectangularArray< T >::operator[])
    .def("rbegin", &RectangularArray< T >::rbegin)
    .def("data", ( typename fetch::memory::RectangularArray< T >::container_type & (RectangularArray< T >::*)() ) &RectangularArray< T >::data)
    .def("data", ( const typename fetch::memory::RectangularArray< T >::container_type & (RectangularArray< T >::*)() const ) &RectangularArray< T >::data)
    .def("Load", &RectangularArray< T >::Load)
    .def("Set", ( const T & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &, const T &) ) &RectangularArray< T >::Set)
    .def("Set", ( const T & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &, const typename fetch::memory::RectangularArray< T >::size_type &, const T &) ) &RectangularArray< T >::Set)
    .def("operator()", ( const typename fetch::memory::RectangularArray< T >::type & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &, const typename fetch::memory::RectangularArray< T >::size_type &) const ) &RectangularArray< T >::operator())
    .def("operator()", ( typename fetch::memory::RectangularArray< T >::type & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &, const typename fetch::memory::RectangularArray< T >::size_type &) ) &RectangularArray< T >::operator())
    .def("Crop", &RectangularArray< T >::Crop)
    .def("At", ( const typename fetch::memory::RectangularArray< T >::type & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &) const ) &RectangularArray< T >::At)
    .def("At", ( const typename fetch::memory::RectangularArray< T >::type & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &, const typename fetch::memory::RectangularArray< T >::size_type &) const ) &RectangularArray< T >::At)
    .def("At", ( T & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &, const typename fetch::memory::RectangularArray< T >::size_type &) ) &RectangularArray< T >::At)
    .def("At", ( T & (RectangularArray< T >::*)(const typename fetch::memory::RectangularArray< T >::size_type &) ) &RectangularArray< T >::At);

}
};
};

#endif
