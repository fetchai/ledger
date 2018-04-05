#ifndef LIBFETCHCORE_CONTAINERS_VECTOR_HPP
#define LIBFETCHCORE_CONTAINERS_VECTOR_HPP
#include "containers/vector.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace containers
{

template< typename T >
void BuildVector(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Vector< T >, std::vector<T>>(module, custom_name )
    .def(py::init<>()) /* No constructors found */
    .def("Insert", &Vector< T >::Insert)
    .def("empty", &Vector< T >::empty)
    .def("capacity", &Vector< T >::capacity)
    .def("swap", &Vector< T >::swap)
    .def("Clear", &Vector< T >::Clear)
    .def("Back", ( fetch::containers::Vector::type & (Vector< T >::*)() ) &Vector< T >::Back)
    .def("Back", ( const fetch::containers::Vector::type & (Vector< T >::*)() const ) &Vector< T >::Back)
    .def("Erase", &Vector< T >::Erase)
    .def("At", ( fetch::containers::Vector::type & (Vector< T >::*)(const std::size_t) ) &Vector< T >::At)
    .def("At", ( const fetch::containers::Vector::type & (Vector< T >::*)(const std::size_t) const ) &Vector< T >::At)
    .def("operator[]", ( fetch::containers::Vector::type & (Vector< T >::*)(const std::size_t) ) &Vector< T >::operator[])
    .def("operator[]", ( const fetch::containers::Vector::type & (Vector< T >::*)(const std::size_t) const ) &Vector< T >::operator[])
    .def("PopBack", &Vector< T >::PopBack)
    .def("Front", ( fetch::containers::Vector::type & (Vector< T >::*)() ) &Vector< T >::Front)
    .def("Front", ( const fetch::containers::Vector::type & (Vector< T >::*)() const ) &Vector< T >::Front)
    .def("size", &Vector< T >::size)
    .def("PushBack", &Vector< T >::PushBack)
    .def("Resize", &Vector< T >::Resize)
    .def("Reserve", &Vector< T >::Reserve);

}
};
};

#endif