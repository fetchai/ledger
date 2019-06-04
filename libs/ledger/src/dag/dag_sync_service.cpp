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

using namespace fetch::ledger;

using DAGNodesSerializer        = fetch::serializers::ByteArrayBuffer;

DAGSyncService::DAGSyncService(Muddle &muddle, std::shared_ptr<ledger::DAG> dag)
  : muddle::rpc::Server(muddle.AsEndpoint(), 980, 981)
  , muddle_(muddle)
  , client_(std::make_shared<Client>("R:DAGSync-L",
                                     muddle_.AsEndpoint(), Muddle::Address(), SERVICE_DAG, CHANNEL_RPC))
  , state_machine_{std::make_shared<core::StateMachine<State>>("DAGSyncService",
                                                               State::INITIAL)}
  , dag_{dag}
  , dag_subscription_(muddle_.AsEndpoint().Subscribe(SERVICE_DAG, CHANNEL_RPC_BROADCAST))
{
  state_machine_->RegisterHandler(State::INITIAL, this, &DAGSyncService::OnInitial);
  state_machine_->RegisterHandler(State::BROADCAST_RECENT, this, &DAGSyncService::OnBroadcastRecent);
  state_machine_->RegisterHandler(State::ADD_BROADCAST_RECENT, this, &DAGSyncService::OnAddBroadcastRecent);
  state_machine_->RegisterHandler(State::QUERY_MISSING, this, &DAGSyncService::OnQueryMissing);
  state_machine_->RegisterHandler(State::RESOLVE_MISSING, this, &DAGSyncService::OnResolveMissing);

  state_machine_->OnStateChange([](State /*new_state*/, State /* old_state */) { });

  // Broadcast blocks arrive here
  dag_subscription_->SetMessageHandler([this](Address const &from, uint16_t, uint16_t, uint16_t,
                                                muddle::Packet::Payload const &payload,
                                                Address                transmitter) {

      FETCH_UNUSED(from);
      FETCH_UNUSED(transmitter);

      std::cerr << "argha size: " << payload.size() << std::endl; // DELETEME_NH

      DAGNodesSerializer serialiser(payload);

      std::vector<DAGNode> result;
      serialiser >> result;

      std::cerr << "************************ RECV broadcast of size " << result.size() << std::endl; // DELETEME_NH

      std::cerr << "recv hash: " << result[0].hash.ToBase64() << std::endl; // DELETEME_NH

      std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
      this->recvd_broadcast_nodes_.push_back(std::move(result));
  });
}

DAGSyncService::~DAGSyncService()
{
}

DAGSyncService::State DAGSyncService::OnInitial(){  

  if (muddle_.AsEndpoint().GetDirectlyConnectedPeers().empty())
  {
    state_machine_->Delay(std::chrono::milliseconds{700});
    return State::INITIAL;
  }

  return State::BROADCAST_RECENT;
}

DAGSyncService::State DAGSyncService::OnBroadcastRecent()
{
  std::vector<DAGNode> nodes_to_broadcast = dag_->GetRecentlyAdded();

  if(!nodes_to_broadcast.empty())
  {
    std::cerr << "about to broadcast: " << nodes_to_broadcast.size() << std::endl; // DELETEME_NH
    std::cerr << "broadcast " << nodes_to_broadcast[0].hash.ToBase64() << std::endl; // DELETEME_NH

    // determine the serialised size of the dag nodes to send
    fetch::serializers::SizeCounter<std::vector<DAGNode>> counter;
    counter << nodes_to_broadcast;

    // allocate the buffer and serialise the dag nodes to send
    DAGNodesSerializer serializer;
    serializer.Reserve(counter.size());
    serializer << nodes_to_broadcast;

    std::cerr << "counter size: " << counter.size() << std::endl; // DELETEME_NH
    std::cerr << "data size: " << serializer.data().size() << std::endl; // DELETEME_NH

    // broadcast the block to the nodes on the network
    muddle_.AsEndpoint().Broadcast(SERVICE_DAG, CHANNEL_RPC_BROADCAST, serializer.data());
  }
  else
  {
    state_machine_->Delay(std::chrono::milliseconds{500});
  }

  return State::ADD_BROADCAST_RECENT;
}

DAGSyncService::State DAGSyncService::OnAddBroadcastRecent()
{
  std::vector<std::vector<DAGNode>> recvd_broadcast_nodes;

  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    recvd_broadcast_nodes = std::move(recvd_broadcast_nodes_);
  }

  for(auto const &nodes_vector : recvd_broadcast_nodes)
  {
    for(auto const &node : nodes_vector)
    {
      dag_->AddDAGNode(node);
    }
  }

  return State::QUERY_MISSING;
}

DAGSyncService::State DAGSyncService::OnQueryMissing()
{
  for (auto const &connection : muddle_.AsEndpoint().GetDirectlyConnectedPeers())
  {
    auto missing = dag_->GetRecentlyMissing();

    // TODO(HUT): comment out
    if(!missing.empty() && !false)
    {
      std::cerr << "requesting " << missing.size() << " missing txs" << std::endl; // DELETEME_NH

      auto promise = PromiseOfMissingNodes(client_->CallSpecificAddress(connection, RPC_DAG_STORE_SYNC, DAGSyncProtocol::REQUEST_NODES, missing));

      missing_pending_.Add(connection, promise);
    }

    // Only request from one peer for now
    break;
  }

  state_machine_->Delay(std::chrono::milliseconds{500});
  return State::RESOLVE_MISSING;
}

DAGSyncService::State DAGSyncService::OnResolveMissing()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto counts = missing_pending_.Resolve();
  FETCH_UNUSED(counts);

  for (auto &result : missing_pending_.Get(MAX_OBJECT_RESOLUTION_PER_CYCLE))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "DAG got ", result.promised.size(), " nodes!");

    for (auto &dag_node : result.promised)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node hash: ", dag_node.hash.ToBase64());
      dag_->AddDAGNode(dag_node);
    }
  }

  return State::BROADCAST_RECENT;
}

