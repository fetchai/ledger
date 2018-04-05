#ifndef LIBFETCHCORE_ABSTRACT_MUTEX_HPP
#define LIBFETCHCORE_ABSTRACT_MUTEX_HPP
#include "abstract_mutex.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace mutex
{

void BuildAbstractMutex(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractMutex, std::mutex>(module, "AbstractMutex" )
    .def(py::init<  >())
    .def("thread_id", &AbstractMutex::thread_id)
    .def("AsString", &AbstractMutex::AsString);

}
};
};

#endif