#ifndef LIBFETCHCORE_CRYPTO_FNV_HPP
#define LIBFETCHCORE_CRYPTO_FNV_HPP
#include "crypto/fnv.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace crypto
{

void BuildFNV(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<FNV, fetch::crypto::StreamHasher>(module, "FNV" )
    .def(py::init<  >())
    .def("Reset", &FNV::Reset)
    .def("uint_digest", &FNV::uint_digest)
    .def("Update", &FNV::Update)
    .def("digest", &FNV::digest)
    .def("Final", &FNV::Final);

}
};
};

#endif