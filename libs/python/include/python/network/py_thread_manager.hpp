#ifndef LIBFETCHCORE_NETWORK_THREAD_MANAGER_HPP
#define LIBFETCHCORE_NETWORK_THREAD_MANAGER_HPP
#include "network/network_manager.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace network {

void BuildNetworkManager(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<NetworkManager>(module, "NetworkManager")
      .def(py::init<std::size_t>())
      .def("io_service", &NetworkManager::io_service)
      .def("OnBeforeStart", &NetworkManager::OnBeforeStart)
      .def("OnAfterStop", &NetworkManager::OnAfterStop)
      .def("OnAfterStart", &NetworkManager::OnAfterStart)
      .def("Stop", &NetworkManager::Stop)
      .def("OnBeforeStop", &NetworkManager::OnBeforeStop)
      .def("Start", &NetworkManager::Start)
      .def("Off", &NetworkManager::Off);
}
};  // namespace network
};  // namespace fetch

#endif
