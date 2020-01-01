#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/mutex.hpp"
#include "logging/logging.hpp"
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
  using ConnectionType       = typename AbstractConnection::SharedType;
  using ConnectionHandleType = typename AbstractConnection::ConnectionHandleType;

  static constexpr char const *LOGGING_NAME = "ClientManager";

  explicit ClientManager(AbstractNetworkServer &server)
    : server_(server)
  {}

  ConnectionHandleType Join(ConnectionType const &client)
  {
    ConnectionHandleType handle = client->handle();
    FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is joining");

    FETCH_LOCK(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  // TODO(issue 28): may be risky if handle type is made small
  void Leave(ConnectionHandleType handle)
  {
    FETCH_LOCK(clients_mutex_);

    auto client{clients_.find(handle)};
    if (client != clients_.end())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is leaving");
      clients_.erase(client);
    }
  }

  bool Send(ConnectionHandleType client, MessageBuffer const &msg)
  {
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

  void Broadcast(MessageBuffer const &msg)
  {
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

  void PushRequest(ConnectionHandleType client, MessageBuffer const &msg)
  {
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

  std::string GetAddress(ConnectionHandleType client)
  {
    FETCH_LOCK(clients_mutex_);
    if (clients_.find(client) != clients_.end())
    {
      return clients_[client]->Address();
    }
    return "0.0.0.0";
  }

private:
  AbstractNetworkServer &                        server_;
  std::map<ConnectionHandleType, ConnectionType> clients_;
  Mutex                                          clients_mutex_;
};
}  // namespace network
}  // namespace fetch
