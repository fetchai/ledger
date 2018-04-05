#ifndef LIBFETCHCORE_CHAIN_BLOCK_GENERATOR_HPP
#define LIBFETCHCORE_CHAIN_BLOCK_GENERATOR_HPP
#include "chain/block_generator.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace chain
{

void BuildBlockGenerator(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<BlockGenerator>(module, "BlockGenerator" )
    .def(py::init<>()) /* No constructors found */
    .def("SwitchBranch", &BlockGenerator::SwitchBranch)
    .def("unspent", &BlockGenerator::unspent)
    .def("PrintBlock", &BlockGenerator::PrintBlock)
    .def("GenerateBlock", &BlockGenerator::GenerateBlock)
    .def("set_group_count", &BlockGenerator::set_group_count)
    .def("PushTransactionSummary", &BlockGenerator::PushTransactionSummary)
    .def("PrintTransactionSummary", &BlockGenerator::PrintTransactionSummary);

}
};
};

#endif