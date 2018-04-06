#ifndef LIBFETCHCORE_SERVICE_FUNCTION_HPP
#define LIBFETCHCORE_SERVICE_FUNCTION_HPP
#include "service/function.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace service
{

template< typename F >
void BuildFunction(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Function< F >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}



};
};

#endif
