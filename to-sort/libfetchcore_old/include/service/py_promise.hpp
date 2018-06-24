#ifndef SERVICE_PY_PROMISE_HPP
#define SERVICE_PY_PROMISE_HPP
#include"service/promise.hpp"

namespace fetch
{
namespace service {

void BuildPromise(pybind11::module &m)
{
  namespace py = pybind11;
  
  py::class_<Promise>(m, "Promise")
    .def(py::init<>())
    
    .def("Wait", &Promise::Wait)
    .def("is_fulfilled", &Promise::is_fulfilled)
    .def("has_failed", &Promise::has_failed)
    .def("is_connection_closed", &Promise::is_connection_closed);


}
  
};
};


#endif
