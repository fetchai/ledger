#ifndef LIBFETCHCORE_NETWORK_THREAD_MANAGER_HPP
#define LIBFETCHCORE_NETWORK_THREAD_MANAGER_HPP
#include "network/thread_manager.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace network
{

void BuildThreadManager(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ThreadManager>(module, "ThreadManager" )
    .def(py::init< std::size_t >())
    .def("io_service", &ThreadManager::io_service)
    .def("OnBeforeStart", &ThreadManager::OnBeforeStart)
    .def("OnAfterStop", &ThreadManager::OnAfterStop)
    .def("OnAfterStart", &ThreadManager::OnAfterStart)
    .def("Stop", &ThreadManager::Stop)
    .def("OnBeforeStop", &ThreadManager::OnBeforeStop)
    .def("Start", &ThreadManager::Start)
    .def("Off", &ThreadManager::Off);

}
};
};

#endif