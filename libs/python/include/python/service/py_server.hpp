#pragma once
#include "service/server.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

template <typename T>
void BuildServiceServer(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ServiceServer<T>, T, fetch::service::ServiceServerInterface>(module, custom_name)
      .def(
          py::init<uint16_t, typename fetch::service::ServiceServer<T>::network_manager_ptr_type>())
      .def("ServiceInterfaceOf", &ServiceServer<T>::ServiceInterfaceOf);
}
};  // namespace service
};  // namespace fetch
