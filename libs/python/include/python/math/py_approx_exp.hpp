#pragma once

#include "math/approx_exp.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <uint8_t N, uint64_t C, bool O>
void BuildExp(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ApproxExp<N, C, O>>(module, custom_name.c_str())
      .def(py::init<const ApproxExp<N, C, O> &>())
      .def(py::init<>())
      //    .def(py::self = py::self )
      .def("SetCoefficient", &ApproxExp<N, C, O>::SetCoefficient);
}
};  // namespace math
};  // namespace fetch
