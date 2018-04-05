#ifndef LIBFETCHCORE_CHAIN_BLOCK_HPP
#define LIBFETCHCORE_CHAIN_BLOCK_HPP
#include "chain/block.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace chain
{

template< typename P, typename H >
void BuildBasicBlock(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<BasicBlock< P, H >, std::enable_shared_from_this<BasicBlock<P, H> >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */
    .def("body", &BasicBlock< P, H >::body)
    .def("block_number", &BasicBlock< P, H >::block_number)
    .def("SetBody", &BasicBlock< P, H >::SetBody)
    .def("weight", ( const double & (BasicBlock< P, H >::*)() const ) &BasicBlock< P, H >::weight)
    .def("weight", ( double & (BasicBlock< P, H >::*)() ) &BasicBlock< P, H >::weight)
    .def("set_weight", &BasicBlock< P, H >::set_weight)
    .def("set_is_loose", &BasicBlock< P, H >::set_is_loose)
    .def("set_block_number", &BasicBlock< P, H >::set_block_number)
    .def("set_previous", &BasicBlock< P, H >::set_previous)
    .def("shared_block", &BasicBlock< P, H >::shared_block)
    .def("header", &BasicBlock< P, H >::header)
    .def("set_total_weight", &BasicBlock< P, H >::set_total_weight)
    .def("is_loose", &BasicBlock< P, H >::is_loose)
    .def("total_weight", ( const double & (BasicBlock< P, H >::*)() const ) &BasicBlock< P, H >::total_weight)
    .def("total_weight", ( double & (BasicBlock< P, H >::*)() ) &BasicBlock< P, H >::total_weight)
    .def("proof", ( const fetch::chain::BasicBlock::proof_type & (BasicBlock< P, H >::*)() const ) &BasicBlock< P, H >::proof)
    .def("proof", ( fetch::chain::BasicBlock::proof_type & (BasicBlock< P, H >::*)() ) &BasicBlock< P, H >::proof)
    .def("is_verified", &BasicBlock< P, H >::is_verified)
    .def("id", &BasicBlock< P, H >::id)
    .def("set_id", &BasicBlock< P, H >::set_id)
    .def("previous", &BasicBlock< P, H >::previous);

}
};
};

#endif