#pragma once
#include "abstract_mutex.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace mutex {

void BuildAbstractMutex(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<AbstractMutex, std::mutex>(module, "AbstractMutex")
      .def(py::init<>())
      .def("thread_id", &AbstractMutex::thread_id)
      .def("AsString", &AbstractMutex::AsString);
}
};  // namespace mutex
};  // namespace fetch
