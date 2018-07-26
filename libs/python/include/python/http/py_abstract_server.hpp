#ifndef LIBFETCHCORE_HTTP_ABSTRACT_SERVER_HPP
#define LIBFETCHCORE_HTTP_ABSTRACT_SERVER_HPP
#include "http/abstract_server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildAbstractHTTPServer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<AbstractHTTPServer>(module, "AbstractHTTPServer")
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace http
};  // namespace fetch

#endif
