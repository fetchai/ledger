#ifndef NODE_DIRECTORY_HPP
#define NODE_DIRECTORY_HPP

// This file holds and manages connections to other nodes

#include "core/logger.hpp"
#include "network/service/client.hpp"
#include "network/service/server.hpp"
#include "ledger/chain/transaction.hpp"
#include "./protocols/fetch_protocols.hpp"
#include "./protocols/network_benchmark/commands.hpp"
#include "./network_classes.hpp"
#include "../tests/include/helper_functions.hpp"

#include <set>

namespace fetch
{
namespace network_benchmark
{

class NodeDirectory
{
public:

  using clientType = service::ServiceClient;

  NodeDirectory(network::ThreadManager tm) :
  tm_{tm}
  {}

  NodeDirectory(NodeDirectory &rhs)            = delete;
  NodeDirectory(NodeDirectory &&rhs)           = delete;
  NodeDirectory operator=(NodeDirectory& rhs)  = delete;
  NodeDirectory operator=(NodeDirectory&& rhs) = delete;

  ~NodeDirectory()
  {
    for(auto &i : serviceClients_)
    {
      delete i.second;
    }
  }

  // Only call this during node setup (not thread safe)
  void AddEndpoint(const Endpoint &endpoint)
  {
    LOG_STACK_TRACE_POINT;
    if (serviceClients_.find(endpoint) == serviceClients_.end())
    {
      fetch::network::TCPClient connection(tm_);
      connection.Connect(endpoint.IP(), endpoint.TCPPort());
      
      auto client = new clientType(connection, tm_);  
      

      serviceClients_[endpoint] = client;
    }
  }

  // temporarily replicate invite functionality for easier debugging
  void InviteAllForw(block_hash const &blockHash, block_type &block)
  {
    LOG_STACK_TRACE_POINT;

    for(auto &i : serviceClients_)
    {
      auto client = i.second;

      if(!client->is_alive())
      {
        std::cerr << "Client has died (forw)!\n\n" << std::endl;
      }

      bool clientWants = client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
          protocols::NetworkBenchmark::INVITE_PUSH, blockHash);

      if(clientWants)
      {
        fetch::logger.Info("Client wants forwarded push");
        client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
          protocols::NetworkBenchmark::PUSH, blockHash, block);
      }
    }
  }

  void InviteAllDirect(block_hash const &blockHash, block_type const &block)
  {
    LOG_STACK_TRACE_POINT;

    for(auto &i : serviceClients_)
    {
      auto client = i.second;

      if(!client->is_alive())
      {
        std::cerr << "Client has died!\n\n" << std::endl;
        exit(1);
      }

      client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
        protocols::NetworkBenchmark::PUSH_CONFIDENT, blockHash, block);
    }
  }

  void InviteAllBlocking(block_hash const &blockHash, block_type const &block)
  {
    LOG_STACK_TRACE_POINT;

    for(auto &i : serviceClients_)
    {
      auto client = i.second;

      if(!client->is_alive())
      {
        std::cerr << "Client has died!\n\n" << std::endl;
        exit(1);
      }

      auto p1 = client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
        protocols::NetworkBenchmark::PUSH_CONFIDENT, blockHash, block);
      p1.Wait();
    }
  }

  void ControlSlaves()
  {
    LOG_STACK_TRACE_POINT;

    for(auto &i : serviceClients_)
    {
      auto client = i.second;

      if(!client->is_alive())
      {
        std::cerr << "Client to slave has died!\n\n" << std::endl;
        exit(1);
      }

      while(client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
        protocols::NetworkBenchmark::SEND_NEXT).As<bool>()) {}
    }
  }

  void Reset()
  {
    for(auto &i : serviceClients_)
    {
      delete i.second;
    }
    serviceClients_.clear();
  }

private:
  fetch::network::ThreadManager            tm_;
  std::map<Endpoint, clientType *>         serviceClients_;
};

}
}
#endif
