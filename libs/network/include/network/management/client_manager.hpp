#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
  using connection_type        = typename AbstractConnection::shared_type;
  using connection_handle_type = typename AbstractConnection::connection_handle_type;

  static constexpr char const *LOGGING_NAME = "ClientManager";

  ClientManager(AbstractNetworkServer &server)
    : server_(server)
    , clients_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;
  }

  connection_handle_type Join(connection_type client)
  {
    LOG_STACK_TRACE_POINT;
    connection_handle_type handle = client->handle();
    FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is joining");

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  // TODO(issue 28): may be risky if handle type is made small
  void Leave(connection_handle_type handle)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);

    auto client{clients_.find(handle)};
    if (client != clients_.end())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is leaving");
      clients_.erase(client);
    }
  }

  bool Send(connection_handle_type client, message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    bool ret = true;
    clients_mutex_.lock();

    auto which{clients_.find(client)};
    if (which != clients_.end())
    {
      auto c = which->second;
      clients_mutex_.unlock();
      c->Send(msg);
      FETCH_LOG_DEBUG(LOGGING_NAME, "Client manager did send message to ", client);
      clients_mutex_.lock();
    }
    else
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Client not found.");
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
    try
    {
      server_.PushRequest(client, msg);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Error processing packet from ", client, " error: ", ex.what());
      throw;
    }
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
