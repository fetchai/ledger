#ifndef LIBFETCHCORE_SERVICE_PUBLICATION_FEED_HPP
#define LIBFETCHCORE_SERVICE_PUBLICATION_FEED_HPP
#include "service/publication_feed.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace service
{

void BuildHasPublicationFeed(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<HasPublicationFeed, fetch::service::AbstractPublicationFeed>(module, "HasPublicationFeed" )
    .def(py::init< const std::size_t & >())
    .def("create_publisher", &HasPublicationFeed::create_publisher);

}
};
};

#endif