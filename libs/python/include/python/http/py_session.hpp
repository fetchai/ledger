#pragma once
#include "http/session.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace http
{

void BuildSession(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Session>(module, "Session" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

