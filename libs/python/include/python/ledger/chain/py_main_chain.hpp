#ifndef PY_MAIN_CHAIN_HPP
#define PY_MAIN_CHAIN_HPP

#include <iostream>
#include <string>

#include "ledger/chain/main_chain.hpp"

namespace fetch
{
namespace chain
{

void BuildMainChain(pybind11::module &module) {
//  pybind11::class_<fetch::chain::MainChain::block_type>(module, "MainChainBlock")
//    .def(pybind11::init<>())
//    ;
//  pybind11::class_<fetch::chain::MainChain, std::shared_ptr<fetch::chain::MainChain>>(module, "MainChain")
//    .def(pybind11::init<fetch::chain::MainChain::block_type &>())
//    .def("AddBlock", &MainChain::AddBlock)
//    .def("HeaviestBlock", &MainChain::HeaviestBlock)
//    .def("totalBlocks", &MainChain::totalBlocks)
//    ;
}

}
}

#endif //__PY_MAIN_CHAIN_HPP
