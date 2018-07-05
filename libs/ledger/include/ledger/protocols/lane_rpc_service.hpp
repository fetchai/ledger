#ifndef FETCH_LANE_RPC_SERVICE_HPP
#define FETCH_LANE_RPC_SERVICE_HPP

#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "ledger/lane.hpp"
#include "ledger/protocols/state_database_rpc_protocol.hpp"

namespace fetch {
namespace ledger {

/**
 * The RPC service for the lane
 */
class LaneRpcService : public fetch::service::ServiceServer<fetch::network::TCPServer> {
public:

  LaneRpcService(uint16_t port, thread_manager_type thread_manager)
    : ServiceServer(port, thread_manager)
    , state_protocol_(lane_.state_database()) {

    Add(protocols::FetchProtocols::STATE_DATABASE, &state_protocol_);
  }

private:

  Lane lane_{};
  StateDatabaseRpcProtocol state_protocol_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_LANE_RPC_SERVICE_HPP
