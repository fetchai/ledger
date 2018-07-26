#pragma once
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
