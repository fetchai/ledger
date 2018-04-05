#ifndef LIBFETCHCORE_MUTEX_HPP
#define LIBFETCHCORE_MUTEX_HPP
#include "mutex.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace mutex
{

void BuildProductionMutex(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ProductionMutex, fetch::mutex::AbstractMutex>(module, "ProductionMutex" )
    .def(py::init< int, std::string >())
    .def(py::init<  >());

}

void BuildDebugMutex(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<DebugMutex, fetch::mutex::AbstractMutex>(module, "DebugMutex" )
    .def(py::init< int, std::string >())
    .def(py::init<  >())
    .def(py::self = fetch::mutex::DebugMutex() )
    .def("lock", &DebugMutex::lock)
    .def("filename", &DebugMutex::filename)
    .def("thread_id", &DebugMutex::thread_id)
    .def("unlock", &DebugMutex::unlock)
    .def("line", &DebugMutex::line)
    .def("AsString", &DebugMutex::AsString);

}
};
};

#endif