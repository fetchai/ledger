#ifndef LIBFETCHCORE_SERVICE_ABSTRACT_CALLABLE_HPP
#define LIBFETCHCORE_SERVICE_ABSTRACT_CALLABLE_HPP
#include "service/abstract_callable.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace service
{
namespace details
{

template< typename T, typename arguments >
void BuildPacker(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Packer< T, arguments >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}
};
};
};
namespace fetch
{
namespace service
{

void BuildAbstractCallable(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractCallable>(module, "AbstractCallable" )
    .def(py::init< uint64_t >())
    .def("meta_data", &AbstractCallable::meta_data);

}
};
};

#endif
