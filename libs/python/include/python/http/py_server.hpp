#pragma once
#include "http/server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPServer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPServer, fetch::http::AbstractHTTPServer>(module, "HTTPServer")
      .def(
          py::init<const uint16_t &,
                   const fetch::http::HTTPServer::network_manager_ptr_type &>())
      .def("AddMiddleware",
           (void (HTTPServer::*)(
               const fetch::http::HTTPServer::request_middleware_type &)) &
               HTTPServer::AddMiddleware)
      .def("AddMiddleware",
           (void (HTTPServer::*)(
               const fetch::http::HTTPServer::response_middleware_type &)) &
               HTTPServer::AddMiddleware)
      .def("AddModule", &HTTPServer::AddModule)
      .def("PushRequest", &HTTPServer::PushRequest)
      .def("AddView", &HTTPServer::AddView)
      .def("Accept", &HTTPServer::Accept);
}
};  // namespace http
};  // namespace fetch
