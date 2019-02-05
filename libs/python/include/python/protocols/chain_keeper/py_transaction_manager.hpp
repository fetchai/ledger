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

#include "protocols/chain_keeper/transaction_manager.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace protocols {

void BuildTransactionManager(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<TransactionManager>(module, "TransactionManager")
      .def(py::init<>())
      .def("VerifyAppliedList", &TransactionManager::VerifyAppliedList)
      .def("AddTransaction", &TransactionManager::AddTransaction)
      .def("top", &TransactionManager::top)
      .def("Next", &TransactionManager::Next)
      .def("with_transactions_do", &TransactionManager::with_transactions_do)
      .def("applied_count", &TransactionManager::applied_count)
      .def("LastTransactions", &TransactionManager::LastTransactions)
      .def("AddBulkTransactions", &TransactionManager::AddBulkTransactions)
      .def("NextDigest", &TransactionManager::NextDigest)
      .def("LatestSummaries", &TransactionManager::LatestSummaries)
      .def("set_group", &TransactionManager::set_group)
      .def("size", &TransactionManager::size)
      .def("has_unapplied", &TransactionManager::has_unapplied)
      .def("unapplied_count", &TransactionManager::unapplied_count);
}
};  // namespace protocols
};  // namespace fetch
