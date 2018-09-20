//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <memory>

#include "core/serializers/stl_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/protocols/execution_manager_rpc_client.hpp"
#include "ledger/protocols/execution_manager_rpc_protocol.hpp"

using fetch::service::ServiceClient;
using fetch::byte_array::ConstByteArray;
using fetch::network::NetworkManager;

namespace fetch {
namespace ledger {
namespace {

using NetworkClientPtr = ExecutionManagerRpcClient::NetworkClientPtr;
using NetworkClient    = ExecutionManagerRpcClient::NetworkClient;

NetworkClientPtr CreateConnection(ConstByteArray const &host, uint16_t port,
                                  NetworkManager const &network_manager)
{
  // create and connect service client
  NetworkClientPtr connection = std::make_shared<NetworkClient>(network_manager);
  connection->Connect(host, port);

  return connection;
}

}  // namespace

ExecutionManagerRpcClient::ExecutionManagerRpcClient(ConstByteArray const &host,
                                                     uint16_t const &      port,
                                                     NetworkManager const &network_manager)
  : connection_(CreateConnection(host, port, network_manager))
  , service_(std::make_unique<ServiceClient>(*connection_, network_manager))
{}

ExecutionManagerRpcClient::Status ExecutionManagerRpcClient::Execute(block_type const &block)
{
  auto result = service_->Call(RPC_EXECUTION_MANAGER, ExecutionManagerRpcProtocol::EXECUTE, block);
  return result->As<Status>();
}

ExecutionManagerInterface::block_digest_type ExecutionManagerRpcClient::LastProcessedBlock()
{
  auto result =
      service_->Call(RPC_EXECUTION_MANAGER, ExecutionManagerRpcProtocol::LAST_PROCESSED_BLOCK);
  return result->As<block_digest_type>();
}

bool ExecutionManagerRpcClient::IsActive()
{
  auto result = service_->Call(RPC_EXECUTION_MANAGER, ExecutionManagerRpcProtocol::IS_ACTIVE);
  return result->As<bool>();
}

bool ExecutionManagerRpcClient::IsIdle()
{
  auto result = service_->Call(RPC_EXECUTION_MANAGER, ExecutionManagerRpcProtocol::IS_IDLE);
  return result->As<bool>();
}

bool ExecutionManagerRpcClient::Abort()
{
  auto result = service_->Call(RPC_EXECUTION_MANAGER, ExecutionManagerRpcProtocol::ABORT);
  return result->As<bool>();
}

}  // namespace ledger
}  // namespace fetch
