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

#include "crypto/ecdsa.hpp"
#include "network/management/network_manager.hpp"
#include "ledger/dag/dag.hpp"
#include "ledger/dag/dag_node.hpp"
#include "ledger/dag/dag_muddle_configuration.hpp"
#include "ledger/protocols/dag_rpc_protocol.hpp"


#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/packet.hpp"
#include "network/service/protocol.hpp"
#include "crypto/fnv.hpp"

#include "variant/variant.hpp"
#include "core/byte_array/encoders.hpp"

#include <atomic>
#include <memory>
#include <deque>

namespace fetch {
namespace ledger {

class DAGRpcService : public muddle::rpc::Server,
                      public std::enable_shared_from_this<DAGRpcService>
{
public:
  static constexpr char const *LOGGING_NAME = "DAGRpcService"; 

  using MuddleEndpoint  = muddle::MuddleEndpoint;  
  using Muddle          = muddle::Muddle;
  using SubscriptionPtr = muddle::MuddleEndpoint::SubscriptionPtr;
  using VerifierType    = crypto::ECDSAVerifier;
  using ThreadPool      = network::ThreadPool;
  using NodeList        = std::vector<DAGNode>;

  struct QueueItem {
    DAGNode node;
    int attempts = 0;
    // TODO: add from adddress such that trust system can penalise bad behaviour
  };

  using NodeQueue       = std::deque<QueueItem>;
  using Packet = fetch::muddle::Packet;
  using DAGNodeAddedCallback = std::function< void(DAGNode const &) >;

  enum 
  {
    DAG_CHUNK_SIZE = 100000
  };

  // Construction / Destruction
  DAGRpcService(Muddle &muddle, MuddleEndpoint &endpoint, DAG &dag) 
  : muddle::rpc::Server(endpoint, DAG_RPC_SERVICE, CHANNEL_DAG_RPC)
  , muddle_(muddle)
  , endpoint_(endpoint)
  , dag_(dag)
  , dag_protocol_(dag)
  , dag_subscription_(endpoint.Subscribe(DAG_RPC_SERVICE, CHANNEL_DAG))
  {
    LOG_STACK_TRACE_POINT;

    thread_pool_ = network::MakeThreadPool(1, "DAG Thread Pool");

    this->Add(DAG_SYNCRONISATION, &dag_protocol_); // TODO: Use shared pointers

    dag_subscription_->SetMessageHandler([this](Address const &from, uint16_t, uint16_t, uint16_t,
                                                Packet::Payload const &payload,
                                                Address                transmitter)  {
        DAGNode node = UnpackNode(payload);
        thread_pool_->Post([this, node]() { 

          AddNodeToQueue(node); 
        });
        
      });

  }

  void Start() 
  {
    // Worker thread
    // TODO: Move to separate start function
    thread_pool_->Start();
    thread_pool_->Post([this]() { IdleUntilWork(); });
  }

  void Stop()
  {
    thread_pool_->Stop();
  }


  void IdleUntilWork() 
  {
    LOG_STACK_TRACE_POINT;
    
    if(!syncronising_)
    {
      std::this_thread::sleep_for( std::chrono::milliseconds(100));
      thread_pool_->Post([this]() { AddUrgentNodes(); });        
    }
    else
    {
      std::this_thread::sleep_for( std::chrono::milliseconds(1000));
      thread_pool_->Post([this]() { IdleUntilWork(); });        
    }

  }

  void AddUrgentNodes() 
  {
    LOG_STACK_TRACE_POINT;
    thread_pool_->Post([this]() { AddBacklogNodes(); });    
  }  

  void AddBacklogNodes() 
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


    thread_pool_->Post([this]() { AddNewNodes(); });
  }    

  void AddNewNodes() 
  {
    LOG_STACK_TRACE_POINT;
    int n;
    {
      FETCH_LOCK(normal_queue_mutex_);
      n = int(normal_node_queue_.size());
    }

//    FETCH_LOG_INFO(LOGGING_NAME, "Processing ", n, "DAG nodes");
    while(n > 0)
    {
      QueueItem item;

      {
        FETCH_LOCK(normal_queue_mutex_);        
        item = normal_node_queue_.front();
        normal_node_queue_.pop_front();
      }
//      FETCH_LOG_INFO(LOGGING_NAME, "Processing DAG node: ", byte_array::ToBase64(item.node.hash));

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

    thread_pool_->Post([this]() { IdleUntilWork(); });    
  } 


  DAGRpcService(DAGRpcService const &) = delete;
  DAGRpcService(DAGRpcService &&) = delete;
  ~DAGRpcService() = default;

  void BroadcastDAGNode(DAGNode node) 
  { 
    LOG_STACK_TRACE_POINT;
    fetch::serializers::TypedByteArrayBuffer buf;
    buf << node;
    
    // TODO(tfr): Move constants to where they belong
    endpoint_.Broadcast(fetch::ledger::DAG_RPC_SERVICE, fetch::ledger::CHANNEL_DAG, buf.data());
  } 


  void Synchronise()
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
//      FETCH_LOG_INFO(LOGGING_NAME, "Making call");
      promises.push_back(client->Call(muddle_.network_id().value(), DAG_SYNCRONISATION, DAGProtocol::NUMBER_OF_DAG_NODES));
//      FETCH_LOG_INFO(LOGGING_NAME, "DONE!");      
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


  void SetCertificate(byte_array::ConstByteArray const &private_key)
  {
    LOG_STACK_TRACE_POINT;
    certificate_ = std::make_shared<fetch::crypto::ECDSASigner>();
    certificate_->Load(private_key);
  }

  // Operators
  DAGRpcService &operator=(DAGRpcService const &) = delete;
  DAGRpcService &operator=(DAGRpcService &&) = delete;

  void SignalNewDAGNode(DAGNode node, bool broadcast = true) 
  { 
    LOG_STACK_TRACE_POINT;
    if(!dag_.HasNode(node.hash))
    {
      dag_.Push(node);
      if(broadcast)
      {
        BroadcastDAGNode(node);
      }
    }
  } 

private:
  using SignerPtr         = std::shared_ptr<crypto::ECDSASigner>;
  using RpcServerPtr      = std::unique_ptr<muddle::rpc::Server>;
  using DAGProtocolPtr    = std::unique_ptr<DAGProtocol>;
  using ClientPtr         = std::shared_ptr<muddle::rpc::Client>;
  using Flag              = std::atomic< bool >;

  bool AddNodeToQueue(DAGNode node)
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

  DAGNode UnpackNode(byte_array::ConstByteArray const &msg)
  {
    LOG_STACK_TRACE_POINT;
    DAGNode                           ret;
    
    serializers::TypedByteArrayBuffer buf(msg);
    buf >> ret;

    return ret;
  }

  mutable fetch::mutex::Mutex global_mutex_{__LINE__, __FILE__};

  mutable fetch::mutex::Mutex urgent_queue_mutex_{__LINE__, __FILE__};
  NodeQueue urgent_node_queue_;

  mutable fetch::mutex::Mutex backlog_queue_mutex_{__LINE__, __FILE__};
  NodeQueue backlog_node_queue_;

  mutable fetch::mutex::Mutex normal_queue_mutex_{__LINE__, __FILE__};
  NodeQueue normal_node_queue_;

  Flag syncronising_{false};


  crypto::Identity identity_;
  RpcServerPtr      server_;
  DAGProtocolPtr protocol_;
  SignerPtr         certificate_;
  ThreadPool thread_pool_;

  /// @name DAG 
  /// @{
  Muddle &muddle_;
  MuddleEndpoint &endpoint_;
  DAG               &dag_;
  DAGProtocol       dag_protocol_;

  SubscriptionPtr      dag_subscription_;
  DAGNodeAddedCallback on_dag_node_added_;  
  /// @}

};

}
}
