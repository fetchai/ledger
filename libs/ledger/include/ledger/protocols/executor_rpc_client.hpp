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

#include <memory>

#include "core/serializers/stl_types.hpp"
#include "ledger/executor_interface.hpp"
#include "ledger/protocols/executor_rpc_protocol.hpp"
#include "network/protocols/fetch_protocols.hpp"
#include "network/service/client.hpp"
#include "network/tcp/tcp_client.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcClient : public ExecutorInterface
{
public:
  using connection_type      = network::TCPClient;
  using service_type         = std::unique_ptr<service::ServiceClient>;
  using network_manager_type = service::ServiceClient::network_manager_type;

  ExecutorRpcClient(byte_array::ConstByteArray const &host, uint16_t const &port,
                    network_manager_type const &network_manager)
  {

    // create the connection
    connection_type connection{network_manager};
    service_ = std::make_unique<service::ServiceClient>(connection, network_manager);

    connection.Connect(host, port);
  }

  Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) override
  {
    auto result = service_->Call(protocols::FetchProtocols::EXECUTOR, ExecutorRpcProtocol::EXECUTE,
                                 hash, slice, lanes);
    return result.As<Status>();
  }

  bool is_alive() const { return service_->is_alive(); }

private:
  service_type service_;
};

}  // namespace ledger
}  // namespace fetch
