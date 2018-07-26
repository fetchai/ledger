#pragma once
#include "crypto/prover.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace crypto {

void BuildProver(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Prover>(module, "Prover")
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace crypto
};  // namespace fetch
