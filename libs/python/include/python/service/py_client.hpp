#pragma once
#include "service/client.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace service
{

template< typename T >
void BuildServiceClient(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<ServiceClient< T >, T, fetch::service::ServiceClientInterface, fetch::service::ServiceServerInterface>(module, custom_name )
    .def(py::init< const std::string &, const uint16_t &, typename fetch::service::ServiceClient< T >::network_manager_ptr_type >())
    .def("PushMessage", &ServiceClient< T >::PushMessage)
    .def("ConnectionFailed", &ServiceClient< T >::ConnectionFailed);

}
};
};

