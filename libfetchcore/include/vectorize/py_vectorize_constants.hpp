#ifndef LIBFETCHCORE_VECTORIZE_VECTORIZE_CONSTANTS_HPP
#define LIBFETCHCORE_VECTORIZE_VECTORIZE_CONSTANTS_HPP
#include "vectorize/vectorize_constants.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace vectorize
{

template< uint16_t I, typename T >
void BuildRegisterInfo(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<RegisterInfo< I, T >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif