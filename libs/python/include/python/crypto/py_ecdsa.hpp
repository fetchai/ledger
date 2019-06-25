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

#include "crypto/ecdsa.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace crypto {

void BuildECDSASigner(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ECDSASigner, fetch::crypto::Prover>(module, "ECDSASigner")
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
};  // namespace crypto
};  // namespace fetch
