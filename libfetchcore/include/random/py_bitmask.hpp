#ifndef LIBFETCHCORE_RANDOM_BITMASK_HPP
#define LIBFETCHCORE_RANDOM_BITMASK_HPP
#include "random/bitmask.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace random
{

template< typename W, int B, bool MSBF >
void BuildBitMask(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<BitMask< W, B, MSBF >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */
    .def("SetProbability", &BitMask< W, B, MSBF >::SetProbability);

}
};
};

#endif
