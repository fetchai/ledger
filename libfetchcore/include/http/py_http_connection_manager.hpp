#ifndef LIBFETCHCORE_HTTP_HTTP_CONNECTION_MANAGER_HPP
#define LIBFETCHCORE_HTTP_HTTP_CONNECTION_MANAGER_HPP
#include "http/http_connection_manager.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace http
{

void BuildHTTPConnectionManager(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<HTTPConnectionManager>(module, "HTTPConnectionManager" )
    .def(py::init< fetch::http::AbstractHTTPServer & >())
    .def("Leave", &HTTPConnectionManager::Leave)
    .def("PushRequest", &HTTPConnectionManager::PushRequest)
    .def("Join", &HTTPConnectionManager::Join)
    .def("GetAddress", &HTTPConnectionManager::GetAddress)
    .def("Send", &HTTPConnectionManager::Send);

}
};
};

#endif