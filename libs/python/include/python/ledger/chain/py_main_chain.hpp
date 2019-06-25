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

#include <string>

#include "ledger/chain/main_chain.hpp"

namespace fetch {
namespace ledger {

void BuildMainChain(pybind11::module &module)
{
  //  pybind11::class_<fetch::ledger::MainChain::block_type>(module,
  //  "MainChainBlock")
  //    .def(pybind11::init<>())
  //    ;
  //  pybind11::class_<fetch::ledger::MainChain,
  //  std::shared_ptr<fetch::ledger::MainChain>>(module, "MainChain")
  //    .def(pybind11::init<fetch::ledger::MainChain::block_type &>())
  //    .def("AddBlock", &MainChain::AddBlock)
  //    .def("HeaviestBlock", &MainChain::HeaviestBlock)
  //    .def("totalBlocks", &MainChain::totalBlocks)
  //    ;
}

}  // namespace ledger
}  // namespace fetch
