#ifndef LIBFETCHCORE_SCRIPT_DICTIONARY_HPP
#define LIBFETCHCORE_SCRIPT_DICTIONARY_HPP
#include "script/dictionary.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace script
{

template< typename T >
void BuildDictionary(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Dictionary< T >>(module, custom_name )
    .def(py::init<  >())
    .def(py::init< const Dictionary<T> & >())
    .def(py::init< Dictionary<T> && >())
    .def("begin", ( typename container_type::iterator (Dictionary< T >::*)() ) &Dictionary< T >::begin)
    .def("begin", ( typename container_type::const_iterator (Dictionary< T >::*)() const ) &Dictionary< T >::begin)
    .def("Copy", &Dictionary< T >::Copy)
    .def("end", ( typename container_type::iterator (Dictionary< T >::*)() ) &Dictionary< T >::end)
    .def("end", ( typename container_type::const_iterator (Dictionary< T >::*)() const ) &Dictionary< T >::end)
    .def("operator[]", ( fetch::script::Dictionary::type & (Dictionary< T >::*)(const byte_array::BasicByteArray &) ) &Dictionary< T >::operator[])
    .def("operator[]", ( const fetch::script::Dictionary::type & (Dictionary< T >::*)(const byte_array::BasicByteArray &) const ) &Dictionary< T >::operator[]);

}
};
};

#endif