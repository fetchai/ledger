#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/reactor.hpp"
#include "ledger/dag/dag_sync_protocol.hpp"
#include "ledger/dag/dag_sync_service.hpp"
#include "ledger/shard_config.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/server.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class DAGService
{
public:
  static constexpr char const *LOGGING_NAME = "DAGService";

  enum class Mode
  {
    CREATE_DATABASE,
    LOAD_DATABASE
  };

  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<Subscription>;
  using NetworkManager  = network::NetworkManager;
  using MuddleEndpoint  = muddle::MuddleEndpoint;
  using DAGPtr          = std::shared_ptr<ledger::DAGInterface>;

  // Construction / Destruction
  DAGService(MuddleEndpoint &muddle_endpoint, DAGPtr dag, Mode mode = Mode::LOAD_DATABASE);
  DAGService(DAGService const &) = delete;
  DAGService(DAGService &&)      = delete;
  ~DAGService();

  core::WeakRunnable GetWeakRunnable()
  {
    return dag_sync_service_->GetWeakRunnable();
  }

private:
  using DAGSyncProtoPtr   = std::shared_ptr<ledger::DAGSyncProtocol>;
  using DAGSyncServicePtr = std::shared_ptr<ledger::DAGSyncService>;

  using Server    = fetch::muddle::rpc::Server;
  using Reactor   = core::Reactor;
  using ServerPtr = std::shared_ptr<Server>;

  Reactor reactor_;

  /// @name External P2P Network
  /// @{
  ServerPtr       external_rpc_server_;
  MuddleEndpoint &external_muddle_;  ///< The muddle networking service
  /// @}

  /// @name DAG Store sync mechanism
  /// @{
  DAGPtr            dag_;
  SubscriptionPtr   dag_subscription_;
  DAGSyncProtoPtr   dag_sync_protocol_;
  DAGSyncServicePtr dag_sync_service_;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
