#include "ledger/protocols/state_database_rpc_client.hpp"
#include "ledger/protocols/state_database_rpc_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"

namespace fetch {
namespace ledger {

StateDatabaseRpcClient::StateDatabaseRpcClient(byte_array::ConstByteArray const &host,
                                               uint16_t const &port,
                                               network::ThreadManager const &thread_manager) {
  network::TCPClient connection(thread_manager);
  connection.Connect(host, port);
  service_.reset( new fetch::service::ServiceClient(connection, thread_manager ) );  
}

StateDatabaseRpcClient::document_type StateDatabaseRpcClient::GetOrCreate(resource_id_type const &rid) {
  auto result_promise = service_->Call(
    protocols::FetchProtocols::STATE_DATABASE,
    StateDatabaseRpcProtocol::RPC_ID_GET_OR_CREATE,
    rid
  );

  return result_promise.As<document_type>();
}

StateDatabaseRpcClient::document_type StateDatabaseRpcClient::Get(resource_id_type const &rid) {
  auto result_promise = service_->Call(
    protocols::FetchProtocols::STATE_DATABASE,
    StateDatabaseRpcProtocol::RPC_ID_GET,
    rid
  );

  return result_promise.As<document_type>();
}

void StateDatabaseRpcClient::Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) {
  auto result_promise = service_->Call(
    protocols::FetchProtocols::STATE_DATABASE,
    StateDatabaseRpcProtocol::RPC_ID_SET,
    rid,
    value
  );

  result_promise.Wait();
}

StateDatabaseRpcClient::bookmark_type StateDatabaseRpcClient::Commit(bookmark_type const& b) {
  auto result_promise = service_->Call(
    protocols::FetchProtocols::STATE_DATABASE,
    StateDatabaseRpcProtocol::RPC_ID_COMMIT,
    b
  );

  return result_promise.As<bookmark_type>();
}

void StateDatabaseRpcClient::Revert(bookmark_type const &b) {
  auto result_promise = service_->Call(
    protocols::FetchProtocols::STATE_DATABASE,
    StateDatabaseRpcProtocol::RPC_ID_REVERT,
    b
  );

  result_promise.Wait();
}

} // namespace ledger
} // namespace fetch
