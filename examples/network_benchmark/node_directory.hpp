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
  void addEndpoint(const Endpoint &endpoint)
  {
    if (serviceClients_.find(endpoint) == serviceClients_.end())
    {
      auto client = new clientType {endpoint.IP(), endpoint.TCPPort(), tm_};
      serviceClients_[endpoint] = client;
    }
  }

  template <typename T>
  void BroadcastTransactions(T&& trans)
  {
    CallAllEndpoints(protocols::NetworkBenchmark::SEND_TRANSACTIONS, std::forward<T>(trans));
  }

  template<typename T, typename... Args>
  void CallAllEndpoints(T CallEnum, Args... args) // one
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    for(auto &i : serviceClients_)
    {
      auto client = i.second;
      if(!client->is_alive())
      {
        delete client;
        client = new clientType {i.first.IP(), i.first.TCPPort(), tm_};
        //serviceClients_[i] = client;
        fetch::logger.Info(std::cerr, "Forced to recreate client for: ", i.first.IP());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
      client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK, CallEnum, args...);
    }
  }

  std::vector<std::vector<chain::Transaction>> RequestTransactions()
  {
    std::vector<std::vector<chain::Transaction>> result;
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    for(auto &i : serviceClients_) {
      auto client = i.second;
      std::vector<chain::Transaction> res;
      client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK, protocols::NetworkBenchmark::PULL_TRANSACTIONS).As(res);
      result.emplace_back(std::move(res));
    }

    return result;
  }

private:
  fetch::network::ThreadManager    *tm_;
  std::map<Endpoint, clientType *> serviceClients_;
  fetch::mutex::Mutex              mutex_;
};

}
}
#endif
