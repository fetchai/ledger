#ifndef LIBFETCHCORE_SERVICE_CLIENT_HPP
#define LIBFETCHCORE_SERVICE_CLIENT_HPP
#include "service/client.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace service
{

template< typename T >
void BuildServiceClient(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<ServiceClient< T >, T, fetch::service::ServiceClientInterface, fetch::service::ServiceServerInterface>(module, custom_name )
    .def(py::init< const std::string &, const uint16_t &, typename fetch::service::ServiceClient< T >::thread_manager_ptr_type >())
    .def("PushMessage", &ServiceClient< T >::PushMessage)
    .def("ConnectionFailed", &ServiceClient< T >::ConnectionFailed);

}
};
};

#endif
