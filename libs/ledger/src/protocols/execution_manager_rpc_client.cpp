//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
#include "ledger/protocols/execution_manager_rpc_client.hpp"
#include "ledger/protocols/execution_manager_rpc_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"

namespace fetch {
namespace ledger {

ExecutionManagerRpcClient::ExecutionManagerRpcClient(byte_array::ConstByteArray const &host,
                                                     uint16_t const &                  port,
                                                     network::NetworkManager const &network_manager)

{
  network::TCPClient connection(network_manager);
  connection.Connect(host, port);

  service_ = std::make_unique<fetch::service::ServiceClient>(connection, network_manager);
}

ExecutionManagerRpcClient::Status ExecutionManagerRpcClient::Execute(block_type const &block)
{
  auto result = service_->Call(fetch::protocols::FetchProtocols::EXECUTION_MANAGER,
                               ExecutionManagerRpcProtocol::EXECUTE, block);

  return result.As<Status>();
}

ExecutionManagerInterface::block_digest_type ExecutionManagerRpcClient::LastProcessedBlock()
{
  auto result = service_->Call(protocols::FetchProtocols::EXECUTION_MANAGER,
                               ExecutionManagerRpcProtocol::LAST_PROCESSED_BLOCK);

  return result.As<block_digest_type>();
}

bool ExecutionManagerRpcClient::IsActive()
{
  auto result = service_->Call(protocols::FetchProtocols::EXECUTION_MANAGER,
                               ExecutionManagerRpcProtocol::IS_ACTIVE);

  return result.As<bool>();
}

bool ExecutionManagerRpcClient::IsIdle()
{
  auto result = service_->Call(protocols::FetchProtocols::EXECUTION_MANAGER,
                               ExecutionManagerRpcProtocol::IS_IDLE);

  return result.As<bool>();
}

bool ExecutionManagerRpcClient::Abort()
{
  auto result = service_->Call(protocols::FetchProtocols::EXECUTION_MANAGER,
                               ExecutionManagerRpcProtocol::ABORT);

  return result.As<bool>();
}

}  // namespace ledger
}  // namespace fetch
