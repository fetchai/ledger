#pragma once
#include "service/server_interface.hpp"

#include"fetch_pybind.hpp"

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

