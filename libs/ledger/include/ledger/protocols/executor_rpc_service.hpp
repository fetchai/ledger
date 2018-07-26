#ifndef FETCH_EXECUTOR_RPC_SERVICE_HPP
#define FETCH_EXECUTOR_RPC_SERVICE_HPP

#include "ledger/executor.hpp"
#include "ledger/protocols/executor_rpc_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcService : public service::ServiceServer<network::TCPServer>
{
public:
  using resources_type = Executor::resources_type;

  // Construction / Destruction
  ExecutorRpcService(uint16_t port, network_manager_type const &network_manager,
                     resources_type resources)
      : ServiceServer(port, network_manager), executor_(std::move(resources))
  {
    this->Add(protocols::FetchProtocols::EXECUTOR, &protocol_);
  }
  ~ExecutorRpcService() override = default;

private:
  Executor            executor_;
  ExecutorRpcProtocol protocol_{executor_};
};

}  // namespace ledger
}  // namespace fetch

#endif  // FETCH_EXECUTOR_RPC_SERVICE_HPP
