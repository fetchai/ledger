#ifndef LIBFETCHCORE_OPTIMISATION_INSTANCE_BINARY_PROBLEM_HPP
#define LIBFETCHCORE_OPTIMISATION_INSTANCE_BINARY_PROBLEM_HPP
#include "optimisation/instance/binary_problem.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace optimisers
{

void BuildBinaryProblem(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<BinaryProblem>(module, "BinaryProblem" )
    .def(py::init<>()) /* No constructors found */
    .def("Insert", &BinaryProblem::Insert)
    .def("Resize", &BinaryProblem::Resize)
    .def("energy_offset", &BinaryProblem::energy_offset);

}
};
};

#endif