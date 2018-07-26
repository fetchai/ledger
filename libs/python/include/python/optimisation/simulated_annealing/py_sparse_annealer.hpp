#pragma once
#include "optimisation/simulated_annealing/sparse_annealer.hpp"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
namespace fetch {
namespace optimisers {

void BuildSparseAnnealer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<SparseAnnealer>(module, "SparseAnnealer")
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def("beta", &SparseAnnealer::beta)
      .def("Insert", &SparseAnnealer::Insert)
      .def("SetSweeps", &SparseAnnealer::SetSweeps)
      .def("SetBetaEnd", &SparseAnnealer::SetBetaEnd)
      .def("PrintGraph", &SparseAnnealer::PrintGraph)
      .def("Update", &SparseAnnealer::Update)
      .def("SetBeta", &SparseAnnealer::SetBeta)
      .def("SetBetaStart", &SparseAnnealer::SetBetaStart)
      .def("FindMinimum",
           (double (SparseAnnealer::*)()) & SparseAnnealer::FindMinimum)
      .def("FindMinimum",
           (double (SparseAnnealer::*)(
               fetch::optimisers::SparseAnnealer::state_type &, bool)) &
               SparseAnnealer::FindMinimum)
      .def("CostOf", &SparseAnnealer::CostOf)
      .def("sweeps", &SparseAnnealer::sweeps)
      .def("Anneal", &SparseAnnealer::Anneal)
      .def("Resize", &SparseAnnealer::Resize)
      .def("size", &SparseAnnealer::size);
}
};  // namespace optimisers
};  // namespace fetch
