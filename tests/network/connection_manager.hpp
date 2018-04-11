#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include"network/thread_manager.hpp"
#include"service/client.hpp"

namespace fetch
{
namespace network_test
{

template<typename T>
class ConnectionManager
{
public:

  ConnectionManager(network::ThreadManager *tm) :
    tm_{tm}
  {}

  ~ConnectionManager()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    for(auto &i : serviceClients_)
    {
      delete i.second;
    }
  }

  ConnectionManager(ConnectionManager &rhs)             = delete;
  ConnectionManager(ConnectionManager &&rhs)            = delete;
  ConnectionManager &operator=(ConnectionManager& rhs)  = delete;
  ConnectionManager &operator=(ConnectionManager&& rhs) = delete;

  service::ServiceClient<T> *GetClient(const Endpoint &endpoint)
  {

    mutex_.lock();
    if (serviceClients_.find(endpoint) == serviceClients_.end())
    {
      auto client = new service::ServiceClient<T> {endpoint.IP(), endpoint.TCPPort(), tm_};
      serviceClients_[endpoint] = client;
    }
    mutex_.unlock();

    return serviceClients_[endpoint];
  }

  service::ServiceClient<T> *GetClientFast(const Endpoint &endpoint) const
  {
    return serviceClients_.at(endpoint);
  }

private:
  fetch::network::ThreadManager                   *tm_;
  std::map<Endpoint, service::ServiceClient<T> *> serviceClients_;
  fetch::mutex::Mutex                             mutex_;
};

}
}

#endif /* CONNECTION_MANAGER_HPP */
