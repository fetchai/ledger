#pragma once

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/tcp/abstract_server.hpp"

#include <map>

namespace fetch {
namespace network {

/*
 * ClientManager holds a collection of client objects, almost certainly
 * representing a network connection. Clients are assigned a handle by the
 * server, which uses this to coordinate messages to specific clients
 */

class ClientManager
{
public:
  typedef typename AbstractConnection::shared_type connection_type;
  typedef typename AbstractConnection::connection_handle_type
      connection_handle_type;

  ClientManager(AbstractNetworkServer &server)
      : server_(server), clients_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;
  }

  connection_handle_type Join(connection_type client)
  {
    LOG_STACK_TRACE_POINT;
    connection_handle_type handle = client->handle();
    fetch::logger.Info("Client joining with handle ", handle);

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  // TODO: (`HUT`) : may be risky if handle type is made small
  void Leave(connection_handle_type handle)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);

    if (clients_.find(handle) != clients_.end())
    {
      fetch::logger.Info("Client ", handle, " is leaving");
      clients_.erase(handle);
    }
  }

  bool Send(connection_handle_type client, message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    bool ret = true;
    clients_mutex_.lock();

    if (clients_.find(client) != clients_.end())
    {
      auto c = clients_[client];
      clients_mutex_.unlock();
      c->Send(msg);
      fetch::logger.Debug("Client manager did send message to ", client);
      clients_mutex_.lock();
    }
    else
    {
      fetch::logger.Debug("Client not found.");
      ret = false;
    }
    clients_mutex_.unlock();
    return ret;
  }

  void Broadcast(message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    clients_mutex_.lock();
    for (auto &client : clients_)
    {
      auto c = client.second;
      clients_mutex_.unlock();
      c->Send(msg);
      clients_mutex_.lock();
    }
    clients_mutex_.unlock();
  }

  void PushRequest(connection_handle_type client, message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    server_.PushRequest(client, msg);
  }

  std::string GetAddress(connection_handle_type client)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    if (clients_.find(client) != clients_.end())
    {
      return clients_[client]->Address();
    }
    return "0.0.0.0";
  }

private:
  AbstractNetworkServer &                           server_;
  std::map<connection_handle_type, connection_type> clients_;
  fetch::mutex::Mutex                               clients_mutex_;
};
}  // namespace network
}  // namespace fetch
