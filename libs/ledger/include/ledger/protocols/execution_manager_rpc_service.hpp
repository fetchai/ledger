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

#include "ledger/execution_manager.hpp"
#include "ledger/protocols/execution_manager_rpc_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "network/service/server.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class ExecutionManagerRpcService : public fetch::service::ServiceServer<fetch::network::TCPServer>
{
public:
  using manager_type          = std::shared_ptr<ExecutionManager>;
  using executor_factory_type = ExecutionManager::executor_factory_type;
  using storage_unit_type     = ExecutionManager::storage_unit_type;

  ExecutionManagerRpcService(uint16_t port, network_manager_type const &network_manager,
                             std::size_t num_executors, storage_unit_type storage,
                             executor_factory_type const &factory)
    : ServiceServer(port, network_manager)
    , manager_(new ExecutionManager(num_executors, storage, factory))
  {
    this->Add(fetch::protocols::FetchProtocols::EXECUTION_MANAGER, &protocol_);
  }
  ~ExecutionManagerRpcService() override = default;

  void Start() override
  {
    TCPServer::Start();

    manager_->Start();
  }

  void Stop() override
  {
    manager_->Stop();

    TCPServer::Stop();
  }

  // helpful statistics
  std::size_t completed_executions() const
  {
    return manager_->completed_executions();
  }

private:
  manager_type                manager_;
  ExecutionManagerRpcProtocol protocol_{*manager_};
};

}  // namespace ledger
}  // namespace fetch
