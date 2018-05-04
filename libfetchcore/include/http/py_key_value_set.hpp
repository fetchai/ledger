#ifndef LIBFETCHCORE_HTTP_KEY_VALUE_SET_HPP
#define LIBFETCHCORE_HTTP_KEY_VALUE_SET_HPP
#include "http/key_value_set.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace http
{

void BuildKeyValueSet(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<KeyValueSet, std::map<byte_array::ConstByteArray, byte_array::ConstByteArray>>(module, "KeyValueSet" )
    .def(py::init<>()) /* No constructors found */
    .def("begin", ( typename KeyValueSet::map_type::iterator (KeyValueSet::*)() ) &KeyValueSet::begin)
    .def("begin", ( typename KeyValueSet::map_type::const_iterator (KeyValueSet::*)() const ) &KeyValueSet::begin)
    .def("end", ( typename KeyValueSet::map_type::iterator (KeyValueSet::*)() ) &KeyValueSet::end)
    .def("end", ( typename KeyValueSet::map_type::const_iterator (KeyValueSet::*)() const ) &KeyValueSet::end)
    .def("Clear", &KeyValueSet::Clear)
    .def("cbegin", &KeyValueSet::cbegin)
    .def("Add", ( void (KeyValueSet::*)(const fetch::http::KeyValueSet::byte_array_type &, const fetch::http::KeyValueSet::byte_array_type &) ) &KeyValueSet::Add)
    .def("Add", ( void (KeyValueSet::*)(const fetch::http::KeyValueSet::byte_array_type &, const int &) ) &KeyValueSet::Add)
    .def("operator[]", ( byte_array::ConstByteArray & (KeyValueSet::*)(const byte_array::ConstByteArray &) ) &KeyValueSet::operator[])
    .def("operator[]", ( const byte_array::ConstByteArray & (KeyValueSet::*)(const byte_array::ConstByteArray &) const ) &KeyValueSet::operator[])
    .def("Has", &KeyValueSet::Has)
    .def("cend", &KeyValueSet::cend);

}
};
};

#endif
