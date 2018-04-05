#ifndef LIBFETCHCORE_HTTP_ROUTE_HPP
#define LIBFETCHCORE_HTTP_ROUTE_HPP
#include "http/route.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace http
{

void BuildRoute(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Route>(module, "Route" )
    .def(py::init<>()) /* No constructors found */
    .def("Match", &Route::Match);

}
};
};

#endif