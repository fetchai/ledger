#ifndef LIBFETCHCORE_SERVICE_PROMISE_HPP
#define LIBFETCHCORE_SERVICE_PROMISE_HPP
#include "service/promise.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {
namespace details {

void BuildPromiseImplementation(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<PromiseImplementation>(module, "PromiseImplementation")
      .def(py::init<>())
      .def("exception", &PromiseImplementation::exception)
      .def("has_failed", &PromiseImplementation::has_failed)
      .def("is_connection_closed", &PromiseImplementation::is_connection_closed)
      .def("value", &PromiseImplementation::value)
      .def("Fail", &PromiseImplementation::Fail)
      .def("Fulfill", &PromiseImplementation::Fulfill)
      .def("is_fulfilled", &PromiseImplementation::is_fulfilled)
      .def("id", &PromiseImplementation::id)
      .def("ConnectionFailed", &PromiseImplementation::ConnectionFailed);
}
};  // namespace details
};  // namespace service
};  // namespace fetch
namespace fetch {
namespace service {

void BuildPromise(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Promise>(module, "Promise")
      .def(py::init<>())
      .def("has_failed", &Promise::has_failed)
      .def("reference", &Promise::reference)
      .def("is_connection_closed", &Promise::is_connection_closed)
      .def("is_fulfilled", &Promise::is_fulfilled)
      .def("id", &Promise::id)
      .def("Wait", &Promise::Wait);
}
};  // namespace service
};  // namespace fetch

#endif
