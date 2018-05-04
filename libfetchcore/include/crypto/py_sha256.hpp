#ifndef LIBFETCHCORE_CRYPTO_SHA256_HPP
#define LIBFETCHCORE_CRYPTO_SHA256_HPP
#include "crypto/sha256.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace crypto
{

void BuildSHA256(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<SHA256, fetch::crypto::StreamHasher>(module, "SHA256" )
    .def(py::init<>()) /* No constructors found */
    .def("Reset", &SHA256::Reset)
    .def("Update", &SHA256::Update)
    .def("digest", &SHA256::digest)
    .def("Final", &SHA256::Final);

}
};
};

#endif
