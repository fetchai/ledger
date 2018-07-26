#pragma once
#include "http/route.hpp"

#include"fetch_pybind.hpp"

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

