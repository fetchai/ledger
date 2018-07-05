#ifndef FETCH_EXECUTION_MANAGER_SERVICE_HPP
#define FETCH_EXECUTION_MANAGER_SERVICE_HPP

#include "network/service/server.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/protocols/execution_manager_protocol.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class ExecutionManagerRpcService : public fetch::service::ServiceServer<fetch::network::TCPServer> {
public:

  static const std::size_t DEFAULT_NUM_EXECUTORS = 8;

  using manager_type = std::shared_ptr<ExecutionManager>;

  ExecutionManagerRpcService(uint16_t port, thread_manager_type const &thread_manager)
    : ServiceServer(port, thread_manager) {
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

  manager_type manager_{ std::make_shared<ExecutionManager>(std::size_t(DEFAULT_NUM_EXECUTORS)) };
  ExecutionManagerProtocol protocol_{*manager_};
};


} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_SERVICE_HPP
