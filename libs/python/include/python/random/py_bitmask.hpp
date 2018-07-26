#ifndef LIBFETCHCORE_RANDOM_BITMASK_HPP
#define LIBFETCHCORE_RANDOM_BITMASK_HPP

#include "core/random/bitmask.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace random {

template <typename W, int B, bool MSBF>
void BuildBitMask(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<BitMask<W, B, MSBF>>(module, custom_name.c_str())
      .def(py::init<>()) /* No constructors found */
      .def("SetProbability", &BitMask<W, B, MSBF>::SetProbability);
}
};  // namespace random
};  // namespace fetch

#endif
