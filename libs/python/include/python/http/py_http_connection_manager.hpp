#pragma once
#include "http/http_connection_manager.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPConnectionManager(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPConnectionManager>(module, "HTTPConnectionManager")
      .def(py::init<fetch::http::AbstractHTTPServer &>())
      .def("Leave", &HTTPConnectionManager::Leave)
      .def("PushRequest", &HTTPConnectionManager::PushRequest)
      .def("Join", &HTTPConnectionManager::Join)
      .def("GetAddress", &HTTPConnectionManager::GetAddress)
      .def("Send", &HTTPConnectionManager::Send);
}
};  // namespace http
};  // namespace fetch

