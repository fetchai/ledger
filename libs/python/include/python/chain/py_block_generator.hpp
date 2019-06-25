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

#include "chain/block_generator.hpp"

#include "fetch_pybind.hpp"
namespace fetch {
namespace ledger {

void BuildBlockGenerator(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<BlockGenerator>(module, "BlockGenerator")
      .def(py::init<>()) /* No constructors found */
      .def("SwitchBranch", &BlockGenerator::SwitchBranch)
      .def("unspent", &BlockGenerator::unspent)
      .def("PrintBlock", &BlockGenerator::PrintBlock)
      .def("GenerateBlock", &BlockGenerator::GenerateBlock)
      .def("set_group_count", &BlockGenerator::set_group_count)
      .def("PushTransactionSummary", &BlockGenerator::PushTransactionSummary)
      .def("PrintTransactionSummary", &BlockGenerator::PrintTransactionSummary);
}
};  // namespace ledger
};  // namespace fetch
