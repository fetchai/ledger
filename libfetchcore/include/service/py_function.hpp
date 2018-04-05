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

template< typename U, typename used_args >
void BuildInvoke(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Invoke< U, used_args >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}

template< typename used_args >
void BuildUnrollArguments(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<UnrollArguments< used_args >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}

void BuildFunction(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Function, fetch::service::AbstractCallable>(module, "Function" )
    .def(py::init< fetch::service::Function<void ()>::function_type >())
    .def("operator()", &Function::operator());

}
};
};

#endif