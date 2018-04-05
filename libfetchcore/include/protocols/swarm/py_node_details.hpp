#ifndef LIBFETCHCORE_PROTOCOLS_SWARM_NODE_DETAILS_HPP
#define LIBFETCHCORE_PROTOCOLS_SWARM_NODE_DETAILS_HPP
#include "protocols/swarm/node_details.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace protocols
{

void BuildSharedNodeDetails(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<SharedNodeDetails>(module, "SharedNodeDetails" )
    .def(py::init<  >())
    .def(py::init< const fetch::protocols::SharedNodeDetails & >())
    .def("with_details", &SharedNodeDetails::with_details)
    .def("AddEntryPoint", &SharedNodeDetails::AddEntryPoint)
    .def(py::self == fetch::protocols::SharedNodeDetails() )
    .def("details", &SharedNodeDetails::details)
    .def("default_port", &SharedNodeDetails::default_port)
    .def("default_http_port", &SharedNodeDetails::default_http_port);

}
};
};

#endif