#ifndef LIBFETCHCORE_CRYPTO_PROVER_HPP
#define LIBFETCHCORE_CRYPTO_PROVER_HPP
#include "crypto/prover.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace crypto
{

void BuildProver(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Prover>(module, "Prover" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif