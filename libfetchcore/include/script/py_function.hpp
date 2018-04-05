#ifndef LIBFETCHCORE_SCRIPT_FUNCTION_HPP
#define LIBFETCHCORE_SCRIPT_FUNCTION_HPP
#include "script/function.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace script
{

void BuildFunction(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Function>(module, "Function" )
    .def(py::init<  >())
    .def("PushOperation", &Function::PushOperation)
    .def("operator()", &Function::operator())
    .def("Clear", &Function::Clear)
    .def("size", &Function::size);

}
};
};

#endif