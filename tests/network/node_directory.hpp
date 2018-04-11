#ifndef NODE_DIRECTORY_HPP
#define NODE_DIRECTORY_HPP

// This file holds and manages connections to other nodes

#include"chain/transaction.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/network_test/commands.hpp"
#include"./network_classes.hpp"
#include"./connection_manager.hpp"
#include<set>

namespace fetch
{
namespace network_test
{

class NodeDirectory
{
public:

  NodeDirectory(network::ThreadManager *tm) :
  tm_{tm},
  connectionManger_{tm}
  {}

  NodeDirectory(NodeDirectory &rhs)            = delete;
  NodeDirectory(NodeDirectory &&rhs)           = delete;
  NodeDirectory operator=(NodeDirectory& rhs)  = delete;
  NodeDirectory operator=(NodeDirectory&& rhs) = delete;

  template <typename T>
  void BroadcastTransaction(T&& trans)
  {
    CallAllEndpoints(protocols::NetworkTest::SEND_TRANSACTION, std::forward<T>(trans));
  }

  void addEndpoint(const Endpoint &endpoint)
  {
    endpoints_.insert(endpoint);

    // Request a connection to this new endpoint to avoid flooding
    auto client = connectionManger_.GetClient(endpoint);

    for(auto &i : endpoints_)
    {
      std::cout << i.variant() << std::endl;
    }
  }

  template<typename T, typename... Args>
  void CallAllEndpoints(T CallEnum, Args... args)
  {
    for(auto &i : endpoints_)
    {
      auto client = connectionManger_.GetClientFast(i);
      client->Call(protocols::FetchProtocols::NETWORK_TEST, CallEnum, args...);
    }
  }

private:
  fetch::network::ThreadManager         *tm_;
  std::set<Endpoint>                    endpoints_;
  ConnectionManager<network::TCPClient> connectionManger_;
};

}
}
#endif
