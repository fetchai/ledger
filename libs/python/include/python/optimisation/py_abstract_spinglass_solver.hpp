#pragma once
#include "optimisation/abstract_spinglass_solver.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace optimisers {

void BuildAbstractSpinGlassSolver(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<AbstractSpinGlassSolver>(module, "AbstractSpinGlassSolver")
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace optimisers
};  // namespace fetch
