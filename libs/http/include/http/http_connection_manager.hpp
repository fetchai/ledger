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

#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"
#include "http/abstract_connection.hpp"
#include "http/abstract_server.hpp"

namespace fetch {
namespace http {

class HTTPConnectionManager
{
public:
  using connection_type = typename AbstractHTTPConnection::shared_type;
  using handle_type     = uint64_t;

  static constexpr char const *LOGGING_NAME = "HTTPConnectionManager";

  HTTPConnectionManager(AbstractHTTPServer &server)
    : server_(server)
    , clients_mutex_(__LINE__, __FILE__)
  {}

  handle_type Join(connection_type client)
  {
    LOG_STACK_TRACE_POINT;

    handle_type handle = server_.next_handle();
    FETCH_LOG_DEBUG(LOGGING_NAME, "Client joining with handle ", handle);

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);
    clients_[handle] = client;
    return handle;
  }

  void Leave(handle_type handle)
  {
    LOG_STACK_TRACE_POINT;

    std::lock_guard<fetch::mutex::Mutex> lock(clients_mutex_);

    if (clients_.find(handle) != clients_.end())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is leaving");
      // TODO(issue 35): Close socket!
      clients_.erase(handle);
    }
    FETCH_LOG_DEBUG(LOGGING_NAME, "Client ", handle, " is leaving");
  }

  bool Send(handle_type client, HTTPResponse const &res)
  {
    LOG_STACK_TRACE_POINT;

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

  void PushRequest(handle_type client, HTTPRequest const &req)
  {
    LOG_STACK_TRACE_POINT;

    server_.PushRequest(client, req);
  }

  std::string GetAddress(handle_type client)
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
  AbstractHTTPServer &                   server_;
  std::map<handle_type, connection_type> clients_;
  fetch::mutex::Mutex                    clients_mutex_;
};
}  // namespace http
}  // namespace fetch
