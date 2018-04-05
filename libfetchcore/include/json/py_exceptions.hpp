#ifndef LIBFETCHCORE_JSON_EXCEPTIONS_HPP
#define LIBFETCHCORE_JSON_EXCEPTIONS_HPP
#include "json/exceptions.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace json
{

void BuildUnrecognisedJSONSymbolException(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<UnrecognisedJSONSymbolException, std::exception>(module, "UnrecognisedJSONSymbolException" )
    .def(py::init< const byte_array::Token & >())
    .def("what", &UnrecognisedJSONSymbolException::what);

}
};
};

#endif