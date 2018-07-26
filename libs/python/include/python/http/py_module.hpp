#pragma once
#include "http/module.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPModule(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPModule>(module, "HTTPModule")
      .def(py::init<>()) /* No constructors found */
      .def("Get", &HTTPModule::Get)
      .def("views", &HTTPModule::views)
      .def("Patch", &HTTPModule::Patch)
      .def("AddView", &HTTPModule::AddView)
      .def("Put", &HTTPModule::Put)
      .def("Post", &HTTPModule::Post)
      .def("Delete", &HTTPModule::Delete);
}
};  // namespace http
};  // namespace fetch

