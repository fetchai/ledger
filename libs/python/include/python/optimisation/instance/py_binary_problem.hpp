#pragma once
#include "optimisation/instance/binary_problem.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace optimisers {

void BuildBinaryProblem(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<BinaryProblem>(module, "BinaryProblem")
      .def(py::init<>()) /* No constructors found */
      .def("Insert", &BinaryProblem::Insert)
      .def("Resize", &BinaryProblem::Resize)
      .def("energy_offset", &BinaryProblem::energy_offset);
}
};  // namespace optimisers
};  // namespace fetch

