#ifndef NODE_DIRECTORY_HPP
#define NODE_DIRECTORY_HPP

// This file holds and manages connections to other nodes

#include"chain/transaction.hpp"
#include"./protocols/fetch_protocols.hpp"
#include"./protocols/network_benchmark/commands.hpp"
#include"./network_classes.hpp"
#include"service/client.hpp"
#include<set>

namespace fetch
{
namespace network_benchmark
{

class NodeDirectory
{
public:

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
  void addEndpoint(const Endpoint &endpoint)
  {
    if (serviceClients_.find(endpoint) == serviceClients_.end())
    {
      auto client = new service::ServiceClient<network::TCPClient> {endpoint.IP(), endpoint.TCPPort(), tm_};
      serviceClients_[endpoint] = client;
    }
  }

  void BroadcastTransaction(chain::Transaction trans)
  {
    chain::Transaction trans_temp;
    CallAllEndpoints(trans_temp);
  }

//  template<typename T, typename... Args>
//  void CallAllEndpoints(T CallEnum, Args... args)
//  {
//    for(auto &i : serviceClients_)
//    {
//      auto client = i.second;
//      client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK, CallEnum, args...);
//    }
//  }

  void CallAllEndpoints(chain::Transaction trans)
  {
    for(auto &i : serviceClients_)
    {
      auto client = i.second;
      client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK, protocols::NetworkBenchmark::SEND_TRANSACTION, trans);
    }
  }


  void BroadcastTransaction()
  {
    for(auto &i : serviceClients_)
    {
      auto client = i.second;
      chain::Transaction trans;

      {
        std::stringstream stream;
        stream << "sending a trans" << std::endl;
        std::cerr << stream.str();
      }

      client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK, protocols::NetworkBenchmark::SEND_TRANSACTION, trans);

      {
        std::stringstream stream;
        stream << "sent, a trans" << std::endl;
        std::cerr << stream.str();
      }
    }
  }

private:
  fetch::network::ThreadManager                                    *tm_;
  std::map<Endpoint, service::ServiceClient<network::TCPClient> *> serviceClients_;
};

}
}
#endif
