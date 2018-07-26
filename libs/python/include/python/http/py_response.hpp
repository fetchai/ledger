#ifndef LIBFETCHCORE_HTTP_RESPONSE_HPP
#define LIBFETCHCORE_HTTP_RESPONSE_HPP
#include "http/response.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildHTTPResponse(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<HTTPResponse>(module, "HTTPResponse")
      .def(py::init<byte_array::ByteArray, const fetch::http::MimeType &,
                    const fetch::http::Status &>())
      .def("body", &HTTPResponse::body)
      .def("status", (const fetch::http::Status &(HTTPResponse::*)() const) &
                         HTTPResponse::status)
      .def("status",
           (fetch::http::Status & (HTTPResponse::*)()) & HTTPResponse::status)
      .def("mime_type",
           (const fetch::http::MimeType &(HTTPResponse::*)() const) &
               HTTPResponse::mime_type)
      .def("mime_type", (fetch::http::MimeType & (HTTPResponse::*)()) &
                            HTTPResponse::mime_type)
      .def("header", (const fetch::http::Header &(HTTPResponse::*)() const) &
                         HTTPResponse::header)
      .def("header",
           (fetch::http::Header & (HTTPResponse::*)()) & HTTPResponse::header);
}
};  // namespace http
};  // namespace fetch

#endif
