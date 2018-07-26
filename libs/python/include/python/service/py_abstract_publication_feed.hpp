#pragma once
#include "service/abstract_publication_feed.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace service
{

void BuildAbstractPublicationFeed(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractPublicationFeed>(module, "AbstractPublicationFeed" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

