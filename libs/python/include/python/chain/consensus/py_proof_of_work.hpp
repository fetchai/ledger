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

#include "chain/consensus/proof_of_work.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace ledger {
namespace consensus {

void BuildProofOfWork(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ProofOfWork, math::BigUnsigned>(module, "ProofOfWork")
      .def(py::init<>())
      .def(py::init<fetch::ledger::consensus::ProofOfWork::header_type>())
      .def("target", &ProofOfWork::target)
      .def("SetTarget", &ProofOfWork::SetTarget)
      .def("operator()", &ProofOfWork::operator())
      .def("header", &ProofOfWork::header)
      .def("SetHeader", &ProofOfWork::SetHeader)
      .def("digest", &ProofOfWork::digest);
}
};  // namespace consensus
};  // namespace ledger
};  // namespace fetch
