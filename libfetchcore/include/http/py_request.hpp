#ifndef LIBFETCHCORE_HTTP_REQUEST_HPP
#define LIBFETCHCORE_HTTP_REQUEST_HPP
#include "http/request.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace http
{

void BuildHTTPRequest(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<HTTPRequest>(module, "HTTPRequest" )
    .def(py::init<  >())
    .def("content_length", &HTTPRequest::content_length)
    .def("body", &HTTPRequest::body)
    .def("header_length", &HTTPRequest::header_length)
    .def("protocol", &HTTPRequest::protocol)
    .def("uri", &HTTPRequest::uri)
    .def("header", &HTTPRequest::header)
    .def("JSON", &HTTPRequest::JSON)
    .def("is_valid", &HTTPRequest::is_valid)
    .def("query", &HTTPRequest::query)
    .def("method", &HTTPRequest::method);

}
};
};

#endif