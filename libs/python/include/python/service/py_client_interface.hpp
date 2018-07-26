#pragma once
#include "service/client_interface.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace service
{

void BuildServiceClientInterface(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ServiceClientInterface>(module, "ServiceClientInterface" )
    .def(py::init<  >())
    .def("Subscribe", &ServiceClientInterface::Subscribe)
    .def("CallWithPackedArguments", &ServiceClientInterface::CallWithPackedArguments)
    .def("Unsubscribe", &ServiceClientInterface::Unsubscribe);

}
};
};

