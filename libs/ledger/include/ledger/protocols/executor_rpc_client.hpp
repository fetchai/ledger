#ifndef FETCH_EXECUTOR_RPC_CLIENT_HPP
#define FETCH_EXECUTOR_RPC_CLIENT_HPP

#include "core/serializers/stl_types.hpp"
#include "network/service/client.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "ledger/executor_interface.hpp"
#include "ledger/protocols/executor_rpc_protocol.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcClient : public ExecutorInterface {
public:
  using connection_type = network::TCPClient;
  using service_type = std::unique_ptr<service::ServiceClient>;
  using thread_manager_type = service::ServiceClient::thread_manager_type;

  ExecutorRpcClient(byte_array::ConstByteArray const &host,
                    uint16_t const &port,
                    thread_manager_type const &thread_manager) {

    // create the connection
    connection_type connection{thread_manager};
    service_.reset(new service::ServiceClient{
      connection,
      thread_manager
    });

    connection.Connect(host, port);
  }

  Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) override {
    auto result = service_->Call(protocols::FetchProtocols::EXECUTOR, ExecutorRpcProtocol::EXECUTE, hash, slice, lanes);
    return result.As<Status>();
  }

  bool is_alive() const {
    return service_->is_alive();
  }

private:

  service_type service_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTOR_RPC_CLIENT_HPP
