#ifndef FETCH_EXECUTOR_RPC_CLIENT_HPP
#define FETCH_EXECUTOR_RPC_CLIENT_HPP

#include "network/service/client.hpp"
#include "network/tcp/tcp_client.hpp"
#include "ledger/executor_interface.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcClient : public service::ServiceClient< network::TCPClient >
                        , public ExecutorInterface {
public:

  ExecutorRpcClient(byte_array::ConstByteArray const &host,
                    uint16_t const &port,
                    thread_manager_type const &thread_manager)
    : ServiceClient(host, port, thread_manager) {
  }

  Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) override {
    return CHAIN_CODE_LOOKUP_FAILURE;
  }

};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTOR_RPC_CLIENT_HPP
