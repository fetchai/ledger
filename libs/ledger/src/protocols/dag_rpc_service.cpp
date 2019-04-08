#include "ledger/protocols/dag_rpc_service.hpp"

namespace fetch {
namespace ledger {

DAGRpcService::DAGRpcService(Muddle &muddle, MuddleEndpoint &endpoint, DAG &dag) 
: muddle::rpc::Server(endpoint, DAG_RPC_SERVICE, CHANNEL_DAG_RPC)
, muddle_(muddle)
, endpoint_(endpoint)
, dag_(dag)
, dag_protocol_(dag)
, dag_subscription_(endpoint.Subscribe(DAG_RPC_SERVICE, CHANNEL_DAG))
, state_machine_{std::make_shared<StateMachine>("DAGService", State::WAIT_FOR_NODES,
                                                [](State state) { return ToString(state); })}
{
  LOG_STACK_TRACE_POINT;

  this->Add(DAG_SYNCRONISATION, &dag_protocol_); // TODO: Use shared pointers

  // Preparing state machine
  state_machine_->RegisterHandler(State::WAIT_FOR_NODES,    this, &DAGRpcService::IdleUntilWork);
  state_machine_->RegisterHandler(State::ADD_URGENT_NODES,  this, &DAGRpcService::AddUrgentNodes);
  state_machine_->RegisterHandler(State::ADD_BACKLOG_NODES, this, &DAGRpcService::AddBacklogNodes);
  state_machine_->RegisterHandler(State::ADD_NEW_NODES,     this, &DAGRpcService::AddBacklogNodes);

  // Registering to gosship protocol
  dag_subscription_->SetMessageHandler([this](Address const &, uint16_t, uint16_t, uint16_t,
                                              Packet::Payload const &payload,
                                              Address          )  {
    DAGNode node = UnpackNode(payload);
    AddNodeToQueue(node);  // TODO: Check that this is proctected by a mutex      
  });
}


DAGRpcService::State DAGRpcService::IdleUntilWork() 
{
  std::this_thread::sleep_for( std::chrono::milliseconds(100));

  if(!syncronising_)
  {
    return State::ADD_URGENT_NODES;
  }

  return State::WAIT_FOR_NODES;
}

DAGRpcService::State DAGRpcService::AddUrgentNodes() 
{
  return State::ADD_BACKLOG_NODES;    
}  

DAGRpcService::State DAGRpcService::AddBacklogNodes() 
{
  LOG_STACK_TRACE_POINT;
  int n;
  {
    FETCH_LOCK(backlog_queue_mutex_);
    n = int(backlog_node_queue_.size());
  }

  while(n > 0)
  {
    QueueItem item;

    {
      FETCH_LOCK(backlog_queue_mutex_);        
      item = backlog_node_queue_.front();
      backlog_node_queue_.pop_front();
    }

    if(!dag_.HasNode(item.node.hash))
    {
      if(!dag_.ValidatePrevious(item.node))
      {
        FETCH_LOCK(backlog_queue_mutex_);
        ++item.attempts;
        backlog_node_queue_.push_back(item);
      }
      else
      { // Node accepted and we should propagate.
        if(!dag_.Push(item.node))
        {
          // TODO: Update trust model
        }
      }
    }
    --n;
  }

  return State::ADD_NEW_NODES;
}    

DAGRpcService::State DAGRpcService::AddNewNodes() 
{
  LOG_STACK_TRACE_POINT;
  int n;
  {
    FETCH_LOCK(normal_queue_mutex_);
    n = int(normal_node_queue_.size());
  }

  while(n > 0)
  {
    QueueItem item;

    {
      FETCH_LOCK(normal_queue_mutex_);        
      item = normal_node_queue_.front();
      normal_node_queue_.pop_front();
    }

    if(!dag_.HasNode(item.node.hash))
    {
      if(!dag_.ValidatePrevious(item.node))
      {
        FETCH_LOCK(backlog_queue_mutex_);
        ++item.attempts;
        backlog_node_queue_.push_back(item);
      }
      else
      { // Node accepted and we should propagate.
        if(!dag_.Push(item.node))
        {
          // TODO: Update trust model
        }
      }
    }
    --n;
  }

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


void DAGRpcService::Synchronise()
{
  LOG_STACK_TRACE_POINT;
  syncronising_ =  true;
//    return;
  auto                          connections = muddle_.GetConnections();
  if(connections.size() == 0)
  {
//      ClearDAGBacklog();
    syncronising_ = false;
    return; 
  }

  std::vector<ClientPtr>        clients;
  std::vector<service::Promise> promises;

  for (auto &c: connections)
  {
    auto client = std::make_shared<muddle::rpc::Client>("DAG Sync Client", muddle_.AsEndpoint(), c.first,
                                                        DAG_RPC_SERVICE, CHANNEL_DAG_RPC);
    clients.push_back(client);
    promises.push_back(client->Call(muddle_.network_id().value(), DAG_SYNCRONISATION, DAGProtocol::NUMBER_OF_DAG_NODES));
  }

  if (clients.size() == 0)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "No clients to sync with.");
    syncronising_ = false;
    return;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Syncing with ", connections.size(), " node(s)");

  // Getting the size of the chain
  uint64_t number_of_dag_nodes = 0;
  for (auto &p: promises)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Waiting for promise....");    
    if(p->Wait(2000u))
    {
      number_of_dag_nodes = std::max(number_of_dag_nodes, p->As<uint64_t>());
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "No response from client.");        
    }
  }
  FETCH_LOG_INFO(LOGGING_NAME, "DONE!");    

  // Getting the parts in a Bittorrent type of way
  // TODO(tfr): make offset based on data already existing
  std::unordered_map<uint64_t, service::Promise> dag_chunk_requests;
  uint64_t dag_chunks   = number_of_dag_nodes / DAG_CHUNK_SIZE;

  if (dag_chunks * DAG_CHUNK_SIZE < number_of_dag_nodes)
  {
    ++dag_chunks;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Synchronising ", number_of_dag_nodes, " blocks in ", dag_chunks,
                 " chunks.");

  uint64_t      cid = 0;
  // TODO: Consider the case where one chunk does not come back. This may make that we need to resync

  for (uint64_t i   = 0; i < dag_chunks; ++i)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Call: ", i);      
    dag_chunk_requests[i] = clients[cid]->Call(muddle_.network_id().value(), DAG_SYNCRONISATION, DAGProtocol::DOWNLOAD_DAG, i, uint64_t(DAG_CHUNK_SIZE));
    cid = (cid + 1) % clients.size();
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Adding nodes!");

  // Building DAG
  std::vector<DAGNode> dag_nodes;
  for (uint64_t        i = 0; i < dag_chunks; ++i)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Resolving: ", i," ", dag_chunk_requests.size());            
    dag_nodes.clear();
    dag_chunk_requests[i]->Wait();
    FETCH_LOG_INFO(LOGGING_NAME, "XX: ", int(dag_chunk_requests[i]->GetState()));
    dag_chunk_requests[i]->As(dag_nodes);

    
    for (auto node: dag_nodes)
    {
      AddNodeToQueue(node);
    }

  }

  syncronising_ = false;
  return;
}



bool DAGRpcService::AddNodeToQueue(DAGNode node)
{ 
  FETCH_LOCK(global_mutex_);
  // Ensures that we are not adding nodes we already know
  if(dag_.HasNode(node.hash))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "DAG node already exists: ", byte_array::ToBase64(node.hash));
    return false;
  }

//    FETCH_LOG_INFO(LOGGING_NAME, "Queuing node: ", byte_array::ToBase64(node.hash) );
  if(node.identity.identifier().size() == 0)
  {
    // TODO: work out why this errors comes around.
    std::cout << "TODO: Error in AddNodeToQUeue" << std::endl;
    std::cout << byte_array::ToBase64(node.hash) << std::endl;
    std::cout << node.contents << std::endl;
    return false;
  }

  node.Finalise();
  assert( node.identity.identifier().size() > 0);

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

}
}