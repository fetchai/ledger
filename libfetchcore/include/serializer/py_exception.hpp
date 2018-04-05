#ifndef LIBFETCHCORE_SERIALIZER_EXCEPTION_HPP
#define LIBFETCHCORE_SERIALIZER_EXCEPTION_HPP
#include "serializer/exception.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace serializers
{

void BuildSerializableException(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<SerializableException, std::exception>(module, "SerializableException" )
    .def(py::init<  >())
    .def(py::init< std::string >())
    .def(py::init< error::error_type, std::string >())
    .def("StackTrace", &SerializableException::StackTrace)
    .def("what", &SerializableException::what)
    .def("error_code", &SerializableException::error_code)
    .def("explanation", &SerializableException::explanation);

}
};
};

#endif