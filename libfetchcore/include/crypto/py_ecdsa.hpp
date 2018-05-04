#ifndef LIBFETCHCORE_CRYPTO_ECDSA_HPP
#define LIBFETCHCORE_CRYPTO_ECDSA_HPP
#include "crypto/ecdsa.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace crypto
{

void BuildECDSASigner(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<ECDSASigner, fetch::crypto::Prover>(module, "ECDSASigner" )
    .def(py::init<>()) /* No constructors found */
    .def("Load", &ECDSASigner::Load)
    .def("public_key", &ECDSASigner::public_key)
    .def("private_key", &ECDSASigner::private_key)
    .def("GenerateKeys", &ECDSASigner::GenerateKeys)
    .def("SetPrivateKey", &ECDSASigner::SetPrivateKey)
    .def("Sign", &ECDSASigner::Sign)
    .def("Verify", &ECDSASigner::Verify)
    .def("signature", &ECDSASigner::signature)
    .def("document_hash", &ECDSASigner::document_hash);

}
};
};

#endif
