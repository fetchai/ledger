#include "core/serializers/stl_types.hpp"
#include "ledger/protocols/execution_manager_rpc_client.hpp"
#include "ledger/protocols/execution_manager_rpc_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"

namespace fetch {
namespace ledger {


ExecutionManagerRpcClient::ExecutionManagerRpcClient(byte_array::ConstByteArray const &host,
                                                     uint16_t const &port,
                                                     network::ThreadManager const &thread_manager)
  
{
  network::TCPClient connection(thread_manager);
  connection.Connect(host, port);
  service_.reset( new fetch::service::ServiceClient(connection, thread_manager ) );
}

bool ExecutionManagerRpcClient::Execute(tx_index_type const &index,
                                        block_map_type &map,
                                        std::size_t num_lanes,
                                        std::size_t num_slices)
{
  // make the RPC call
  auto result = service_->Call(fetch::protocols::FetchProtocols::EXECUTION_MANAGER,
                           ExecutionManagerRpcProtocol::RPC_ID_EXECUTE,
                           index, map, num_lanes, num_slices);

  return result.As<bool>();
}

} // namespace ledger
} // namespace fetch
