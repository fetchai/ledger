#pragma once
#include "serializers/exception.hpp"

#include"fetch_pybind.hpp"

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

