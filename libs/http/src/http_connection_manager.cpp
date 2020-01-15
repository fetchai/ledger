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

#include "core/byte_array/byte_array.hpp"
#include "http/abstract_connection.hpp"
#include "http/abstract_server.hpp"
#include "http/http_connection_manager.hpp"
#include "http/request.hpp"
#include "logging/logging.hpp"

#include <iostream>
#include <utility>

namespace fetch {
namespace http {

HTTPConnectionManager::HTTPConnectionManager(AbstractHTTPServer &server)
  : server_(server)
{}

HTTPConnectionManager::~HTTPConnectionManager()
{
  FETCH_LOCK(clients_mutex_);

  for (auto const &client : clients_)
  {
    client.second->CloseConnnection();
  }

  clients_.clear();
}

HTTPConnectionManager::HandleType HTTPConnectionManager::Join(ConnectionType client)
{
  HandleType handle = AbstractHTTPServer::next_handle();
  FETCH_LOG_DEBUG(LOGGING_NAME, "Client joining with handle ", handle);

  FETCH_LOCK(clients_mutex_);
  clients_[handle] = std::move(client);
  return handle;
}

void HTTPConnectionManager::Leave(HandleType handle)
{
  FETCH_LOCK(clients_mutex_);

  auto it = clients_.find(handle);

  if (it != clients_.end())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is leaving");
    it->second->CloseConnnection();
    clients_.erase(it);
  }
  FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is leaving");
}

bool HTTPConnectionManager::Send(HandleType client, HTTPResponse const &res)
{
  bool ret = true;
  clients_mutex_.lock();

  if (clients_.find(client) != clients_.end())
  {
    auto c = clients_[client];
    clients_mutex_.unlock();
    c->Send(res);
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

void HTTPConnectionManager::PushRequest(HandleType client, HTTPRequest const &req)
{
  server_.PushRequest(client, req);
}

std::string HTTPConnectionManager::GetAddress(HandleType client)
{
  FETCH_LOCK(clients_mutex_);
  if (clients_.find(client) != clients_.end())
  {
    return clients_[client]->Address();
  }

  return "0.0.0.0";
}

bool HTTPConnectionManager::has_connections() const
{
  FETCH_LOCK(clients_mutex_);
  return !clients_.empty();
}

}  // namespace http
}  // namespace fetch
