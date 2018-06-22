#ifndef LIBFETCHCORE_MATH_LOG_HPP
#define LIBFETCHCORE_MATH_LOG_HPP

#include"math/log.hpp"
#include"python/fetch_pybind.hpp"

namespace fetch
{
namespace math
{

void BuildLog(pybind11::module &module) {
  /*
  namespace py = pybind11;
  py::class_<Log>(module, "Log" )
    .def(py::init<>()) 
    .def("operator()", &Log::operator());
*/
}
};
};

#endif
