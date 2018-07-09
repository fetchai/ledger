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

bool ExecutionManagerRpcClient::Execute(block_digest_type const &block_hash,
                                        block_digest_type const &prev_block_hash,
                                        tx_index_type const &index,
                                        block_map_type &map, std::size_t num_lanes,
                                        std::size_t num_slices) {
  auto result = service_->Call(
    fetch::protocols::FetchProtocols::EXECUTION_MANAGER,
    ExecutionManagerRpcProtocol::EXECUTE,
    block_hash,
    prev_block_hash,
    index, map,
    num_lanes,
    num_slices
  );

  return result.As<bool>();
}

ExecutionManagerInterface::block_digest_type ExecutionManagerRpcClient::LastProcessedBlock() {
  auto result = service_->Call(
    protocols::FetchProtocols::EXECUTION_MANAGER,
    ExecutionManagerRpcProtocol::LAST_PROCESSED_BLOCK
  );

  return result.As<block_digest_type>();
}

bool ExecutionManagerRpcClient::IsActive() {
  auto result = this->Call(
    protocols::FetchProtocols::EXECUTION_MANAGER,
    ExecutionManagerRpcProtocol::IS_ACTIVE
  );

  return result.As<bool>();
}

bool ExecutionManagerRpcClient::IsIdle() {
  auto result = service_->Call(
    protocols::FetchProtocols::EXECUTION_MANAGER,
    ExecutionManagerRpcProtocol::IS_IDLE
  );

  return result.As<bool>();
}

bool ExecutionManagerRpcClient::Abort() {
  auto result = service_->Call(
    protocols::FetchProtocols::EXECUTION_MANAGER,
    ExecutionManagerRpcProtocol::ABORT
  );

  return result.As<bool>();
}

} // namespace ledger
} // namespace fetch
