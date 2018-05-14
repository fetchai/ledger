#ifndef NETWORK_CLIENT_MANAGER_HPP
#define NETWORK_CLIENT_MANAGER_HPP

#include "assert.hpp"
#include "logger.hpp"
#include "mutex.hpp"
#include "network/tcp/abstract_connection.hpp"
#include "network/tcp/abstract_server.hpp"

#include <map>

namespace fetch {
namespace network {

class ClientManager {
 public:
  typedef typename AbstractClientConnection::shared_type connection_type;
  typedef uint64_t handle_type;  // TODO make global definition

  ClientManager(AbstractNetworkServer& server)
      : server_(server), clients_mutex_(__LINE__, __FILE__) {
    LOG_STACK_TRACE_POINT;
  }

  handle_type Join(connection_type client) {
    LOG_STACK_TRACE_POINT;
    handle_type handle = server_.next_handle();
    fetch::logger.Info("Client joining with handle ", handle);

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  void Leave(handle_type handle) {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);

    if (clients_.find(handle) != clients_.end()) {
      fetch::logger.Info("Client ", handle, " is leaving");
      clients_.erase(handle);
    }
  }

  bool Send(handle_type client, message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    bool ret = true;
    clients_mutex_.lock();

    if (clients_.find(client) != clients_.end()) {
      auto c = clients_[client];
      clients_mutex_.unlock();
      c->Send(msg);
      fetch::logger.Debug("Client manager did send message to ", client);
      clients_mutex_.lock();
    } else {
      fetch::logger.Debug("Client not found.");
      ret = false;
    }
    clients_mutex_.unlock();
    return ret;
  }

  void Broadcast(message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    clients_mutex_.lock();
    for (auto& client : clients_) {
      auto c = client.second;
      clients_mutex_.unlock();
      c->Send(msg);
      clients_mutex_.lock();
    }
    clients_mutex_.unlock();
  }

  void PushRequest(handle_type client, message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    server_.PushRequest(client, msg);
  }

  std::string GetAddress(handle_type client) {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    if (clients_.find(client) != clients_.end()) {
      return clients_[client]->Address();
    }
    return "0.0.0.0";
  }

 private:
  AbstractNetworkServer& server_;
  std::map<handle_type, connection_type> clients_;
  fetch::mutex::Mutex clients_mutex_;
};
}
}

#endif
