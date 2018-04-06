#ifndef LIBFETCHCORE_RANDOM_BITGENERATOR_HPP
#define LIBFETCHCORE_RANDOM_BITGENERATOR_HPP
#include "random/bitgenerator.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace random
{

template< typename R, uint8_t B, bool MSBF >
void BuildBitGenerator(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<BitGenerator< R, B, MSBF >>(module, custom_name.c_str() )
    .def(py::init<>()) /* No constructors found */
    .def("operator()", &BitGenerator< R, B, MSBF >::operator())
    .def("Seed", &BitGenerator< R, B, MSBF >::Seed);

}
};
};

#endif
