#pragma once
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
      .def("chains", (const fetch::protocols::ChainManager::chain_map_type &(
                         ChainManager::*)() const) &
                         ChainManager::chains)
      .def("chains", (fetch::protocols::ChainManager::chain_map_type &
                      (ChainManager::*)()) &
                         ChainManager::chains)
      .def("size", &ChainManager::size);
}
};  // namespace protocols
};  // namespace fetch
