#ifndef LIBFETCHCORE_HTTP_SERVER_HPP
#define LIBFETCHCORE_HTTP_SERVER_HPP
#include "http/server.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace http
{

void BuildHTTPServer(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<HTTPServer, fetch::http::AbstractHTTPServer>(module, "HTTPServer" )
    .def(py::init< const uint16_t &, const fetch::http::HTTPServer::thread_manager_ptr_type & >())
    .def("AddMiddleware", ( void (HTTPServer::*)(const fetch::http::HTTPServer::request_middleware_type &) ) &HTTPServer::AddMiddleware)
    .def("AddMiddleware", ( void (HTTPServer::*)(const fetch::http::HTTPServer::response_middleware_type &) ) &HTTPServer::AddMiddleware)
    .def("AddModule", &HTTPServer::AddModule)
    .def("PushRequest", &HTTPServer::PushRequest)
    .def("AddView", &HTTPServer::AddView)
    .def("Accept", &HTTPServer::Accept);

}
};
};

#endif