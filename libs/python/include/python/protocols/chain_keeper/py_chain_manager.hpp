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

#include "protocols/chain_keeper/chain_manager.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace protocols {

void BuildChainManager(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ChainManager>(module, "ChainManager")
      .def(py::init<>())
      .def("AddBulkBlocks", &ChainManager::AddBulkBlocks)
      .def("AddBlock", &ChainManager::AddBlock)
      .def("latest_blocks", &ChainManager::latest_blocks)
      .def("head", &ChainManager::head)
      .def("set_group", &ChainManager::set_group)
      .def("chains",
           (const fetch::protocols::ChainManager::chain_map_type &(ChainManager::*)() const) &
               ChainManager::chains)
      .def("chains", (fetch::protocols::ChainManager::chain_map_type & (ChainManager::*)()) &
                         ChainManager::chains)
      .def("size", &ChainManager::size);
}
};  // namespace protocols
};  // namespace fetch
