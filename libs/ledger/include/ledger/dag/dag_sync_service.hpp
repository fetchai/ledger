#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "ledger/transaction_verifier.hpp"
#include "ledger/chain/transaction.hpp"
#include "core/state_machine.hpp"
#include "core/service_ids.hpp"
#include "network/muddle/muddle.hpp"

#include "network/generics/requesting_queue.hpp"

#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"

#include "ledger/dag/dag.hpp"

namespace fetch {
namespace ledger {

namespace dag_sync {

enum class State
{
  INITIAL = 0,
  BROADCAST_RECENT,
  ADD_BROADCAST_RECENT,
  QUERY_MISSING,
  RESOLVE_MISSING,
};

}

/**
 * DAG implementation.
 */
class DAGSyncService
{
public:

  static constexpr char const *LOGGING_NAME = "DAGSyncService";

  using Muddle                = muddle::Muddle;
  using TransactionPtr          = std::shared_ptr<Transaction>;

  using Client                = muddle::rpc::Client;
  using ClientPtr             = std::shared_ptr<Client>;
  using State                 = dag_sync::State;
  using StateMachine          = core::StateMachine<State>;
  using SubscriptionPtr       = muddle::MuddleEndpoint::SubscriptionPtr;

  using MissingTXs             = DAG::MissingTXs;
  using MissingNodes           = DAG::MissingNodes;
  using RequestingMissingNodes = network::RequestingQueueOf<muddle::Packet::Address, MissingNodes>;
  using PromiseOfMissingNodes  = network::PromiseOf<MissingNodes>;
  using MuddleEndpoint         = muddle::MuddleEndpoint;

  DAGSyncService(MuddleEndpoint &muddle_endpoint, std::shared_ptr<ledger::DAG> dag);
  ~DAGSyncService();

  static constexpr std::size_t MAX_OBJECT_RESOLUTION_PER_CYCLE       = 128;

  core::WeakRunnable GetWeakRunnable()
  {
    return state_machine_;
  }

private:

  State OnInitial();
  State OnBroadcastRecent();
  State OnAddBroadcastRecent();
  State OnQueryMissing();
  State OnResolveMissing();

  MuddleEndpoint                &muddle_endpoint_;
  ClientPtr                     client_;
  std::shared_ptr<StateMachine> state_machine_;
  std::shared_ptr<ledger::DAG>  dag_;
  SubscriptionPtr               dag_subscription_;

  RequestingMissingNodes missing_pending_;

  fetch::mutex::Mutex               mutex_{__LINE__, __FILE__};
  std::vector<std::vector<DAGNode>> recvd_broadcast_nodes_;
};


}  // namespace ledger
}  // namespace fetch


