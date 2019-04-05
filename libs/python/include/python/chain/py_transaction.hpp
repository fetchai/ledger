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

#include "chain/transaction.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace ledger {

void BuildTransaction(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Transaction>(module, "Transaction")
      .def(py::init<>()) /* No constructors found */
      .def("contract_name", &Transaction::contract_name)
      //    .def("signatures", &Transaction::signatures)
      .def("PushGroup",
           (void (Transaction::*)(const byte_array::ConstByteArray &)) & Transaction::PushGroup)
      .def("PushGroup", (void (Transaction::*)(const group_type &)) & Transaction::PushGroup)
      .def("PushSignature", &Transaction::PushSignature)
      .def("data", &Transaction::data)
      .def("signature_count", &Transaction::signature_count)
      .def("UpdateDigest", &Transaction::UpdateDigest)
      .def("set_arguments", &Transaction::set_arguments)
      .def("arguments", &Transaction::arguments)
      .def("groups", &Transaction::groups)
      .def("UsesGroup", &Transaction::UsesGroup)
      .def("summary", &Transaction::summary)
      .def("set_contract_name", &Transaction::set_contract_name)
      .def("digest", &Transaction::digest);
}
};  // namespace ledger
};  // namespace fetch
