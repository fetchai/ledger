#ifndef NODE_DIRECTORY_HPP
#define NODE_DIRECTORY_HPP

// This file holds and manages connections to other nodes

#include"chain/transaction.hpp"
#include"./protocols/fetch_protocols.hpp"
#include"./protocols/network_benchmark/commands.hpp"
#include"./network_classes.hpp"
#include"../tests/include/helper_functions.hpp"
#include"service/client.hpp"
#include"service/server.hpp"
#include"logger.hpp"
#include<set>

namespace fetch
{
namespace network_benchmark
{

class NodeDirectory
{
public:

  using clientType = service::ServiceClient<network::TCPClient>;

  NodeDirectory(network::ThreadManager *tm) :
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

  // Only call this during node setup
  void AddEndpoint(const Endpoint &endpoint)
  {
    LOG_STACK_TRACE_POINT;
    if (serviceClients_.find(endpoint) == serviceClients_.end())
    {
      auto client = new clientType {endpoint.IP(), endpoint.TCPPort(), tm_};
      serviceClients_[endpoint] = client;
    }
  }

  template<typename T>
  void InviteAllForw(T&& transactions)
  {
    LOG_STACK_TRACE_POINT;

    //std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    for(auto &i : serviceClients_)
    {
      auto client = i.second;

      if(!client->is_alive())
      {
        std::cerr << "Client has died (forw)!\n\n" << std::endl;
      }

      bool clientWants = client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
          protocols::NetworkBenchmark::INVITE_PUSH, transactions.first);

      if(clientWants)
      {
        fetch::logger.Info("Client wants forwarded push");
        client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
          protocols::NetworkBenchmark::PUSH, transactions);
      } else
      {
        fetch::logger.Info("Client does not want forwarded push");
      }
    }
  }

  template<typename T>
  void InviteAllDirect(T&& transactions)
  {
    LOG_STACK_TRACE_POINT;

    //std::lock_guard<fetch::mutex::Mutex> lock(mutex_); // TODO: (`HUT`) : remove
    for(auto &i : serviceClients_)
    {
      auto client = i.second;

      if(!client->is_alive())
      {
        std::cerr << "Client has died!\n\n" << std::endl;
        exit(1);
      }

      bool clientWants = client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
          protocols::NetworkBenchmark::INVITE_PUSH, transactions.first);

      if(clientWants)
      {
        fetch::logger.Info("Client wants push");
        try
        {
          client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
            protocols::NetworkBenchmark::PUSH, transactions);
        }
        catch (std::exception& e)
        {
          std::cerr << "failed to write" << std::endl;
          std::cerr << e.what() << std::endl;
          exit(1);
        }

      } else
      {
        fetch::logger.Info("Client does not want push\n\n\n");
      }
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
  fetch::network::ThreadManager            *tm_;
  std::map<Endpoint, clientType *>         serviceClients_;
  fetch::mutex::Mutex                      mutex_;
};

}
}
#endif
