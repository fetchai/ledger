#pragma once
#include "service/protocol.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace service
{

void BuildProtocol(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Protocol>(module, "Protocol" )
    .def(py::init<>()) /* No constructors found */
    .def("Expose", &Protocol::Expose)
    .def("Subscribe", &Protocol::Subscribe)
    .def("Unsubscribe", &Protocol::Unsubscribe)
    .def("operator[]", &Protocol::operator[])
    .def("feeds", &Protocol::feeds)
    .def("RegisterFeed", &Protocol::RegisterFeed);

}
};
};

