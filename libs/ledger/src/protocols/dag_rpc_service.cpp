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

#include "ledger/protocols/dag_rpc_service.hpp"

namespace fetch {
namespace ledger {

DAGRpcService::DAGRpcService(Muddle &muddle, MuddleEndpoint &endpoint, DAG &dag)
  : muddle::rpc::Server(endpoint, DAG_RPC_SERVICE, CHANNEL_DAG_RPC)
  , state_machine_{std::make_shared<StateMachine>("DAGService", State::WAIT_FOR_NODES,
                                                  [](State state) { return ToString(state); })}
  , muddle_(muddle)
  , endpoint_(endpoint)
  , dag_(dag)
  , dag_protocol_(dag)
  , dag_subscription_(endpoint.Subscribe(DAG_RPC_SERVICE, CHANNEL_DAG))
{
  LOG_STACK_TRACE_POINT;

  (void)(muddle_);

  this->Add(DAG_SYNCRONISATION, &dag_protocol_);  // TODO: Use shared pointers

  // Preparing state machine
  state_machine_->RegisterHandler(State::WAIT_FOR_NODES, this, &DAGRpcService::IdleUntilWork);
  state_machine_->RegisterHandler(State::ADD_URGENT_NODES, this, &DAGRpcService::AddUrgentNodes);
  state_machine_->RegisterHandler(State::ADD_BACKLOG, this, &DAGRpcService::AddBacklog);
  state_machine_->RegisterHandler(State::ADD_NEW_NODES, this, &DAGRpcService::AddNewNodes);

  // Registering to gosship protocol
  dag_subscription_->SetMessageHandler([this](Address const &, uint16_t, uint16_t, uint16_t,
                                              Packet::Payload const &payload, Address) {
    DAGNode node = UnpackNode(payload);
    AddNodeToQueue(node);  // TODO: Check that this is proctected by a mutex
  });
}

DAGRpcService::State DAGRpcService::IdleUntilWork()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  if (!syncronising_)
  {
    return State::ADD_URGENT_NODES;
  }

  return State::WAIT_FOR_NODES;
}

DAGRpcService::State DAGRpcService::AddUrgentNodes()
{
  return State::ADD_BACKLOG;
}

DAGRpcService::State DAGRpcService::AddBacklog()
{

  return State::ADD_NEW_NODES;
}

DAGRpcService::State DAGRpcService::AddNewNodes()
{
  LOG_STACK_TRACE_POINT;

  return State::WAIT_FOR_NODES;
}

void DAGRpcService::BroadcastDAGNode(DAGNode node)
{
  LOG_STACK_TRACE_POINT;
  fetch::serializers::TypedByteArrayBuffer buf;
  buf << node;

  // TODO(tfr): Move constants to where they belong
  endpoint_.Broadcast(fetch::ledger::DAG_RPC_SERVICE, fetch::ledger::CHANNEL_DAG, buf.data());
}

bool DAGRpcService::AddNodeToQueue(DAGNode node)
{
  FETCH_LOCK(global_mutex_);
  // Ensures that we are not adding nodes we already know
  if (dag_.HasNode(node.hash))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "DAG node already exists: ", byte_array::ToBase64(node.hash));
    return false;
  }

  //    FETCH_LOG_INFO(LOGGING_NAME, "Queuing node: ", byte_array::ToBase64(node.hash) );
  if (node.identity.identifier().size() == 0)
  {
    // TODO: work out why this errors comes around.
    std::cout << "TODO: Error in AddNodeToQUeue" << std::endl;
    std::cout << byte_array::ToBase64(node.hash) << std::endl;
    std::cout << node.contents << std::endl;
    return false;
  }

  node.Finalise();
  assert(node.identity.identifier().size() > 0);

  VerifierType verfifier(node.identity);
  if (!verfifier.Verify(node.hash, node.signature))
  {
    // TODO: Update trust system
    return false;
  }
  //    std::cout << "DONE!" << std::endl;

  QueueItem item{};
  item.node = node;

  {
    FETCH_LOCK(normal_queue_mutex_);
    normal_node_queue_.push_back(item);
  }

  return true;
}

}  // namespace ledger
}  // namespace fetch