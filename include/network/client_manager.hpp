#ifndef NETWORK_CLIENT_MANAGER_HPP
#define NETWORK_CLIENT_MANAGER_HPP

#include "network/abstract_connection.hpp"
#include "network/abstract_server.hpp"

#include <map>

namespace fetch {
namespace network {

class ClientManager {
 public:
  typedef typename AbstractClientConnection::shared_type connection_type;
  typedef uint64_t handle_type;

  ClientManager(AbstractNetworkServer& server) : server_(server) {}

  handle_type Join(connection_type client) {
    handle_type handle = server_.next_handle();

    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  void Leave(handle_type handle) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(handle);
  }

  void Send(handle_type client, message_type const& msg) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[client]->Send(msg);
  }

  void PushRequest(handle_type client, message_type const& msg) {
    server_.PushRequest(client, msg);
  }

 private:
  AbstractNetworkServer& server_;
  std::map<handle_type, connection_type> clients_;
  std::mutex clients_mutex_;
};
};
};

#endif
