#pragma once

#include "core/random/bitgenerator.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace random {

template <typename R, uint8_t B, bool MSBF>
void BuildBitGenerator(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<BitGenerator<R, B, MSBF>>(module, custom_name.c_str())
      .def(py::init<>()) /* No constructors found */
      .def("operator()", &BitGenerator<R, B, MSBF>::operator())
      .def("Seed", &BitGenerator<R, B, MSBF>::Seed);
}
};  // namespace random
};  // namespace fetch
