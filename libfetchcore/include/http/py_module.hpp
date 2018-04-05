#ifndef LIBFETCHCORE_HTTP_MODULE_HPP
#define LIBFETCHCORE_HTTP_MODULE_HPP
#include "http/module.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace http
{

void BuildHTTPModule(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<HTTPModule>(module, "HTTPModule" )
    .def(py::init<>()) /* No constructors found */
    .def("Get", &HTTPModule::Get)
    .def("views", &HTTPModule::views)
    .def("Patch", &HTTPModule::Patch)
    .def("AddView", &HTTPModule::AddView)
    .def("Put", &HTTPModule::Put)
    .def("Post", &HTTPModule::Post)
    .def("Delete", &HTTPModule::Delete);

}
};
};

#endif