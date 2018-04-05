#ifndef LIBFETCHCORE_SCRIPT_VARIANT_HPP
#define LIBFETCHCORE_SCRIPT_VARIANT_HPP
#include "script/variant.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace script
{

void BuildVariant(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Variant>(module, "Variant" )
    .def(py::init<>()) /* No constructors found */;

}

void BuildVariant(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Variant>(module, "Variant" )
    .def(py::init<  >())
    .def(py::init< const int64_t & >())
    .def(py::init< const int32_t & >())
    .def(py::init< const int16_t & >())
    .def(py::init< const uint64_t & >())
    .def(py::init< const uint32_t & >())
    .def(py::init< const uint16_t & >())
    .def(py::init< const float & >())
    .def(py::init< const double & >())
    .def(py::init< const char * >())
    .def(py::init< const fetch::script::Variant::byte_array_type & >())
    .def(py::init< const char & >())
    .def(py::init< const std::initializer_list<Variant> & >())
    .def(py::init< const fetch::script::Variant & >())
    .def(py::self == char*() )
    .def(py::self == int() )
    .def(py::self == double() )
    .def("as_dictionary", ( fetch::script::Dictionary<Variant> (Variant::*)() ) &Variant::as_dictionary)
    .def("as_dictionary", ( const fetch::script::Variant::variant_dictionary_type & (Variant::*)() const ) &Variant::as_dictionary)
    .def(py::self = fetch::script::Variant() )
    .def(py::self = std::initializer_list<Variant>() )
    .def(py::self = std::nullptr_t*() )
    .def(py::self = bool() )
    .def(py::self = char() )
    .def(py::self = fetch::script::Variant::byte_array_type() )
    .def("as_byte_array", ( const fetch::script::Variant::byte_array_type & (Variant::*)() const ) &Variant::as_byte_array)
    .def("as_byte_array", ( fetch::script::Variant::byte_array_type & (Variant::*)() ) &Variant::as_byte_array)
    .def("is_null", &Variant::is_null)
    .def("as_array", ( fetch::memory::SharedArray<Variant> (Variant::*)() ) &Variant::as_array)
    .def("as_array", ( const fetch::script::Variant::variant_array_type & (Variant::*)() const ) &Variant::as_array)
    .def("as_bool", ( const bool & (Variant::*)() const ) &Variant::as_bool)
    .def("as_bool", ( bool & (Variant::*)() ) &Variant::as_bool)
    .def("as_int", ( const int64_t & (Variant::*)() const ) &Variant::as_int)
    .def("as_int", ( int64_t & (Variant::*)() ) &Variant::as_int)
    .def("operator[]", ( fetch::script::Variant & (Variant::*)(const std::size_t &) ) &Variant::operator[])
    .def("operator[]", ( const fetch::script::Variant & (Variant::*)(const std::size_t &) const ) &Variant::operator[])
    .def("operator[]", ( fetch::script::Variant & (Variant::*)(const byte_array::BasicByteArray &) ) &Variant::operator[])
    .def("operator[]", ( const fetch::script::Variant & (Variant::*)(const byte_array::BasicByteArray &) const ) &Variant::operator[])
    .def("as_double", ( const double & (Variant::*)() const ) &Variant::as_double)
    .def("as_double", ( double & (Variant::*)() ) &Variant::as_double)
    .def("Copy", &Variant::Copy)
    .def("type", &Variant::type)
    .def("size", &Variant::size);

}
};
};

#endif