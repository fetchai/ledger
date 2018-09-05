#pragma once
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

#include "ledger/execution_manager_interface.hpp"
#include "network/service/client.hpp"
#include "network/tcp/tcp_client.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class ExecutionManagerRpcClient : public ExecutionManagerInterface
{
public:
  using NetworkClient = network::TCPClient;
  using NetworkClientPtr = std::shared_ptr<NetworkClient>;
  using ServicePtr = std::unique_ptr<fetch::service::ServiceClient>;
  using ConstByteArray = byte_array::ConstByteArray;
  using NetworkManager = network::NetworkManager;

  // Construction / Destruction
  ExecutionManagerRpcClient(ConstByteArray const &host, uint16_t const &port,
                            NetworkManager const &network_manager);
  ~ExecutionManagerRpcClient() override = default;

  /// @name Execution Manager Interface
  /// @{
  Status            Execute(block_type const &block) override;
  block_digest_type LastProcessedBlock() override;
  bool              IsActive() override;
  bool              IsIdle() override;
  bool              Abort() override;
  /// @}

  bool is_alive() const { return service_->is_alive(); }

private:
  NetworkClientPtr connection_;
  ServicePtr service_;
};

}  // namespace ledger
}  // namespace fetch
