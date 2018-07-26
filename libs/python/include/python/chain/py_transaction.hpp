#pragma once
#include "chain/transaction.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace chain {

void BuildTransaction(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Transaction>(module, "Transaction")
      .def(py::init<>()) /* No constructors found */
      .def("contract_name", &Transaction::contract_name)
      //    .def("signatures", &Transaction::signatures)
      .def("PushGroup",
           (void (Transaction::*)(const byte_array::ConstByteArray &)) &
               Transaction::PushGroup)
      .def("PushGroup",
           (void (Transaction::*)(const group_type &)) & Transaction::PushGroup)
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
};  // namespace chain
};  // namespace fetch
