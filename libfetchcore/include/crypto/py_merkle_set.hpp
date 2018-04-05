#ifndef LIBFETCHCORE_CRYPTO_MERKLE_SET_HPP
#define LIBFETCHCORE_CRYPTO_MERKLE_SET_HPP
#include "crypto/merkle_set.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace crypto
{

void BuildMerkleSet(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<MerkleSet>(module, "MerkleSet" )
    .def(py::init<  >())
    .def("Insert", &MerkleSet::Insert)
    .def("hash", &MerkleSet::hash);

}
};
};

#endif