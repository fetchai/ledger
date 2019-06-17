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

#include "ledger/dag/dag_sync_service.hpp"
#include "ledger/dag/dag_sync_protocol.hpp"
#include "core/service_ids.hpp"

namespace fetch {
namespace ledger {

using DAGNodesSerializer        = fetch::serializers::ByteArrayBuffer;

DAGSyncService::DAGSyncService(MuddleEndpoint &muddle_endpoint, std::shared_ptr<ledger::DAGInterface> dag)
  : muddle_endpoint_(muddle_endpoint)
  , client_(std::make_shared<Client>("R:DAGSync-L",
                                     muddle_endpoint_, Muddle::Address(), SERVICE_DAG, CHANNEL_RPC))
  , state_machine_{std::make_shared<core::StateMachine<State>>("DAGSyncService",
                                                               State::INITIAL)}
  , dag_{std::move(dag)}
  , dag_subscription_(muddle_endpoint_.Subscribe(SERVICE_DAG, CHANNEL_RPC_BROADCAST))
{
  state_machine_->RegisterHandler(State::INITIAL, this, &DAGSyncService::OnInitial);
  state_machine_->RegisterHandler(State::BROADCAST_RECENT, this, &DAGSyncService::OnBroadcastRecent);
  state_machine_->RegisterHandler(State::ADD_BROADCAST_RECENT, this, &DAGSyncService::OnAddBroadcastRecent);
  state_machine_->RegisterHandler(State::QUERY_MISSING, this, &DAGSyncService::OnQueryMissing);
  state_machine_->RegisterHandler(State::RESOLVE_MISSING, this, &DAGSyncService::OnResolveMissing);

  state_machine_->OnStateChange([this](State current, State previous) {
    FETCH_UNUSED(current);
    FETCH_UNUSED(previous);
    //{
    //  FETCH_LOG_INFO(LOGGING_NAME, "Current state: ", ToString(current),
    //                 " (previous: ", ToString(previous), ")");
    //}
  });

  // Broadcast blocks arrive here
  dag_subscription_->SetMessageHandler([this](muddle::Packet::Address const &from, uint16_t, uint16_t, uint16_t,
                                              muddle::Packet::Payload const &payload,
                                              muddle::Packet::Address                transmitter) {
    FETCH_UNUSED(from);
    FETCH_UNUSED(transmitter);

    DAGNodesSerializer serialiser(payload);

    std::vector<DAGNode> result;
    serialiser >> result;

    FETCH_LOG_INFO(LOGGING_NAME, "Saw ", result.size(), " nodes just there ");

    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    this->recvd_broadcast_nodes_.push_back(std::move(result));
  });
}

DAGSyncService::State DAGSyncService::OnInitial(){

  if (muddle_endpoint_.GetDirectlyConnectedPeers().empty())
  {
    state_machine_->Delay(std::chrono::milliseconds{700});
    return State::INITIAL;
  }

  return State::BROADCAST_RECENT;
}

DAGSyncService::State DAGSyncService::OnBroadcastRecent()
{
  std::vector<DAGNode> nodes_to_broadcast = dag_->GetRecentlyAdded();

  for(auto const &node : nodes_to_broadcast)
  {
    nodes_to_broadcast_.push_back(node);
  }


  if(nodes_to_broadcast_.size() > 5)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Broadcasting out ", nodes_to_broadcast_.size(), " nodes.");

    // determine the serialised size of the dag nodes to send
    fetch::serializers::SizeCounter<std::vector<DAGNode>> counter;
    counter << nodes_to_broadcast_;

    // allocate the buffer and serialise the dag nodes to send
    DAGNodesSerializer serializer;
    serializer.Reserve(counter.size());
    serializer << nodes_to_broadcast_;

    nodes_to_broadcast_.clear();

    // broadcast the block to the nodes on the network
    muddle_endpoint_.Broadcast(SERVICE_DAG, CHANNEL_RPC_BROADCAST, serializer.data());
  }

  return State::ADD_BROADCAST_RECENT;
}

DAGSyncService::State DAGSyncService::OnAddBroadcastRecent()
{
  std::vector<std::vector<DAGNode>> recvd_broadcast_nodes;

  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    recvd_broadcast_nodes = std::move(recvd_broadcast_nodes_);
    recvd_broadcast_nodes_.clear();
  }

  if(!recvd_broadcast_nodes.empty() && !recvd_broadcast_nodes.front().empty())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "ARGH: ", recvd_broadcast_nodes.front().size());
  }

  for(auto const &nodes_vector : recvd_broadcast_nodes)
  {
    for(auto const &node : nodes_vector)
    {
      if(node.Verify())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Adding broadcasted dag node. Of: ", nodes_vector.size());
        dag_->AddDAGNode(node);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to verify broadcast dag node!");
      }
    }
  }

  return State::QUERY_MISSING;
}

DAGSyncService::State DAGSyncService::OnQueryMissing()
{
  auto missing = dag_->GetRecentlyMissing();

  //FETCH_LOG_WARN(LOGGING_NAME, "Missing pending: ", missing_pending_.Size(), " Empty : ", missing_pending_.Empty(), " recently missing: ", missing.size());

  missing_dnodes_.clear();

  if(missing_modulo_++ % 10000 == 0 && missing_pending_.Empty())
  {
    for(auto const &dnode : missing)
    {
      missing_dnodes_.insert(dnode);
    }
  }

  if(!missing_dnodes_.empty())
  {
    for (auto const &connection : muddle_endpoint_.GetDirectlyConnectedPeers())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Requesting: ", missing_dnodes_.size(), " missing dnodes! Peers: ", muddle_endpoint_.GetDirectlyConnectedPeers().size());

      auto promise = PromiseOfMissingNodes(client_->CallSpecificAddress(connection, RPC_DAG_STORE_SYNC, DAGSyncProtocol::REQUEST_NODES, missing_dnodes_));
      missing_pending_.Add(connection, promise);

      FETCH_LOG_WARN(LOGGING_NAME, "Size is now: ", missing_pending_.Size());
    }

    state_machine_->Delay(std::chrono::milliseconds{200});
  }

  return State::RESOLVE_MISSING;
}

DAGSyncService::State DAGSyncService::OnResolveMissing()
{
  auto counts = missing_pending_.Resolve();
  missing_pending_.DiscardFailures();

  FETCH_UNUSED(counts);

  bool found_any = false;

  for (auto &result : missing_pending_.Get(MAX_OBJECT_RESOLUTION_PER_CYCLE))
  {
    found_any = true;
    FETCH_LOG_INFO(LOGGING_NAME, "DAG got ", result.promised.size(), " nodes!");

    auto aa = result.promised;

    for (auto &dag_node : result.promised)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node hash: ", dag_node.hash.ToBase64());

      if(dag_node.Verify())
      {
        dag_->AddDAGNode(dag_node);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node hash: ", dag_node.hash.ToBase64(), " not verified!");
      }
    }
  }

  return State::BROADCAST_RECENT;
}

char const *DAGSyncService::ToString(State state)
{
  char const *text = "Unknown";

  switch (state)
  {
  case State::INITIAL:
    text = "Initial State";
    break;
  case State::BROADCAST_RECENT:
    text = "Broadcasting recent";
    break;
  case State::ADD_BROADCAST_RECENT:
    text = "Add broadcast recent";
    break;
  case State::QUERY_MISSING:
    text = "Query for missing nodes";
    break;
  case State::RESOLVE_MISSING:
    text = "Resolve promise for missing nodes";
    break;
  }

  return text;
}

} // namespace ledger
} // namespace fetch



