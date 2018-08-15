#pragma once
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

#include "ledger/executor.hpp"
#include "ledger/protocols/executor_rpc_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcService : public service::ServiceServer<network::TCPServer>
{
public:
  using resources_type = Executor::resources_type;

  // Construction / Destruction
  ExecutorRpcService(uint16_t port, network_manager_type const &network_manager,
                     resources_type resources)
    : ServiceServer(port, network_manager), executor_(std::move(resources))
  {
    this->Add(protocols::FetchProtocols::EXECUTOR, &protocol_);
  }
  ~ExecutorRpcService() override = default;

private:
  Executor            executor_;
  ExecutorRpcProtocol protocol_{executor_};
};

}  // namespace ledger
}  // namespace fetch
