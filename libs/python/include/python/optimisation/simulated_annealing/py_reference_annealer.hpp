#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "optimisation/simulated_annealing/reference_annealer.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace optimisers {

void BuildReferenceAnnealer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ReferenceAnnealer>(module, "ReferenceAnnealer")
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def("SetBetaStart", &ReferenceAnnealer::SetBetaStart)
      .def("SetSweeps", &ReferenceAnnealer::SetSweeps)
      .def("Set", &ReferenceAnnealer::Set)
      .def("accepted", &ReferenceAnnealer::accepted)
      .def("SetBetaEnd", &ReferenceAnnealer::SetBetaEnd)
      .def("Insert", &ReferenceAnnealer::Insert)
      .def("PrintGraph", &ReferenceAnnealer::PrintGraph)
      .def("operator()", (const fetch::optimisers::ReferenceAnnealer::cost_type &(
                             ReferenceAnnealer::*)(const std::size_t &, const std::size_t &)const) &
                             ReferenceAnnealer::operator())
      .def("operator()", (fetch::optimisers::ReferenceAnnealer::cost_type &
                          (ReferenceAnnealer::*)(const std::size_t &, const std::size_t &)) &
                             ReferenceAnnealer::operator())
      .def("attempts", &ReferenceAnnealer::attempts)
      .def("sweeps", &ReferenceAnnealer::sweeps)
      .def("Update", &ReferenceAnnealer::Update)
      .def("SetBeta", &ReferenceAnnealer::SetBeta)
      .def("beta", &ReferenceAnnealer::beta)
      .def("FindMinimum", (double (ReferenceAnnealer::*)()) & ReferenceAnnealer::FindMinimum)
      .def("FindMinimum", (double (ReferenceAnnealer::*)(
                              fetch::optimisers::ReferenceAnnealer::state_type &, bool)) &
                              ReferenceAnnealer::FindMinimum)
      .def("At", (const fetch::optimisers::ReferenceAnnealer::cost_type &(
                     ReferenceAnnealer::*)(const std::size_t &, const std::size_t &)const) &
                     ReferenceAnnealer::At)
      .def("At", (fetch::optimisers::ReferenceAnnealer::cost_type &
                  (ReferenceAnnealer::*)(const std::size_t &, const std::size_t &)) &
                     ReferenceAnnealer::At)
      .def("Anneal", &ReferenceAnnealer::Anneal)
      .def("CostOf", &ReferenceAnnealer::CostOf)
      .def("Resize", &ReferenceAnnealer::Resize)
      .def("size", &ReferenceAnnealer::size);
}
};  // namespace optimisers
};  // namespace fetch
