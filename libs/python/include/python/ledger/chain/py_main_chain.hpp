#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include <iostream>
#include <string>

#include "ledger/chain/main_chain.hpp"

namespace fetch {
namespace chain {

void BuildMainChain(pybind11::module &module)
{
  //  pybind11::class_<fetch::chain::MainChain::block_type>(module,
  //  "MainChainBlock")
  //    .def(pybind11::init<>())
  //    ;
  //  pybind11::class_<fetch::chain::MainChain,
  //  std::shared_ptr<fetch::chain::MainChain>>(module, "MainChain")
  //    .def(pybind11::init<fetch::chain::MainChain::block_type &>())
  //    .def("AddBlock", &MainChain::AddBlock)
  //    .def("HeaviestBlock", &MainChain::HeaviestBlock)
  //    .def("totalBlocks", &MainChain::totalBlocks)
  //    ;
}

}  // namespace chain
}  // namespace fetch
