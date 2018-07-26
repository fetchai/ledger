#ifndef LIBFETCHCORE_PROTOCOLS_CHAIN_KEEPER_CONTROLLER_HPP
#define LIBFETCHCORE_PROTOCOLS_CHAIN_KEEPER_CONTROLLER_HPP
#include "protocols/chain_keeper/controller.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace protocols {

void BuildChainKeeperController(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ChainKeeperController, fetch::service::HasPublicationFeed>(
      module, "ChainKeeperController")
      .def(py::init<const uint64_t &, network::NetworkManager *,
                    fetch::protocols::EntryPoint &>())
      .def("ConnectTo", &ChainKeeperController::ConnectTo)
      .def("count_outgoing_connections",
           &ChainKeeperController::count_outgoing_connections)
      .def("applied_transaction_count",
           &ChainKeeperController::applied_transaction_count)
      .def("unapplied_transaction_count",
           &ChainKeeperController::unapplied_transaction_count)
      .def("SetGroupNumber", &ChainKeeperController::SetGroupNumber)
      .def("group_number", &ChainKeeperController::group_number)
      .def("transaction_count", &ChainKeeperController::transaction_count)
      .def("with_transactions_do", &ChainKeeperController::with_transactions_do)
      .def("AddBulkTransactions", &ChainKeeperController::AddBulkTransactions)
      //    .def("with_peers_do", ( void
      //    (ChainKeeperController::*)(std::function<void (int, const int &)>) )
      //    &ChainKeeperController::with_peers_do) .def("with_peers_do", ( void
      //    (ChainKeeperController::*)(std::function<void (int)>) )
      //    &ChainKeeperController::with_peers_do)
      .def("GetTransactions", &ChainKeeperController::GetTransactions)
      .def("ListenTo", &ChainKeeperController::ListenTo)
      .def("PushTransaction", &ChainKeeperController::PushTransaction)
      .def("GetSummaries", &ChainKeeperController::GetSummaries)
      .def("Hello", &ChainKeeperController::Hello);
}
};  // namespace protocols
};  // namespace fetch

#endif
