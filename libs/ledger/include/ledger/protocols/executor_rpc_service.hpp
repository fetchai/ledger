#ifndef FETCH_EXECUTOR_RPC_SERVICE_HPP
#define FETCH_EXECUTOR_RPC_SERVICE_HPP

#include "network/tcp/tcp_server.hpp"
#include "network/service/server.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "ledger/executor.hpp"
#include "ledger/protocols/executor_rpc_protocol.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcService : public service::ServiceServer<network::TCPServer> {
public:

  // Construction / Destruction
  ExecutorRpcService(uint16_t port, thread_manager_type const &thread_manager)
    : ServiceServer(port, thread_manager) {
    this->Add(protocols::FetchProtocols::EXECUTOR, &protocol_);
  }
  ~ExecutorRpcService() override = default;

private:

  Executor executor_;
  ExecutorRpcProtocol protocol_{executor_};
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTOR_RPC_SERVICE_HPP
