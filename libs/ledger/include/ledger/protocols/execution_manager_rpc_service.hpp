#pragma once

#include "network/service/server.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/protocols/execution_manager_rpc_protocol.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class ExecutionManagerRpcService : public fetch::service::ServiceServer<fetch::network::TCPServer> {
public:

  using manager_type = std::shared_ptr<ExecutionManager>;
  using executor_factory_type = ExecutionManager::executor_factory_type;
  using storage_unit_type = ExecutionManager::storage_unit_type;

  ExecutionManagerRpcService(uint16_t port,
                             network_manager_type const &network_manager,
                             std::size_t num_executors,
                             storage_unit_type storage,
                             executor_factory_type const &factory)
    : ServiceServer(port, network_manager)
    , manager_(new ExecutionManager(num_executors, storage, factory)) {

    this->Add(fetch::protocols::FetchProtocols::EXECUTION_MANAGER, &protocol_);
    manager_->Start();
  }

  ~ExecutionManagerRpcService() override {
    manager_->Stop();
  }

  // helpful statistics
  std::size_t completed_executions() const {
    return manager_->completed_executions();
  }

private:

  manager_type manager_;
  ExecutionManagerRpcProtocol protocol_{*manager_};
};


} // namespace ledger
} // namespace fetch

