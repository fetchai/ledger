#ifndef LIBFETCHCORE_SERVICE_SERVER_INTERFACE_HPP
#define LIBFETCHCORE_SERVICE_SERVER_INTERFACE_HPP
#include "service/server_interface.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace service
{

void BuildServiceServerInterface(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ServiceServerInterface>(module, "ServiceServerInterface" )
    .def(py::init<>()) /* No constructors found */
    .def("Add", &ServiceServerInterface::Add);

}
};
};

#endif