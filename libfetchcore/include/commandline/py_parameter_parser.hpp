#ifndef LIBFETCHCORE_COMMANDLINE_PARAMETER_PARSER_HPP
#define LIBFETCHCORE_COMMANDLINE_PARAMETER_PARSER_HPP
#include "commandline/parameter_parser.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace commandline
{

void BuildParamsParser(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ParamsParser>(module, "ParamsParser" )
    .def(py::init<>()) /* No constructors found */
    .def("Parse", &ParamsParser::Parse)
    .def("arg_size", &ParamsParser::arg_size)
    .def("GetArg", ( std::string (ParamsParser::*)(const std::size_t &) const ) &ParamsParser::GetArg)
    .def("GetArg", ( std::string (ParamsParser::*)(const std::size_t &, const std::string &) const ) &ParamsParser::GetArg)
    .def("GetParam", &ParamsParser::GetParam);

}
};
};

#endif