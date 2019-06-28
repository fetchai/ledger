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

#include <cstdint>

namespace fetch {
namespace http {

class HTTPConnectionManager
{
public:
  using connection_type = typename AbstractHTTPConnection::shared_type;
  using handle_type     = uint64_t;

  static constexpr char const *LOGGING_NAME = "HTTPConnectionManager";

  explicit HTTPConnectionManager(AbstractHTTPServer &server);

  handle_type Join(connection_type client);
  void        Leave(handle_type handle);
  bool        Send(handle_type client, HTTPResponse const &res);
  void        PushRequest(handle_type client, HTTPRequest const &req);
  std::string GetAddress(handle_type client);

private:
  AbstractHTTPServer &                   server_;
  std::map<handle_type, connection_type> clients_;
  fetch::mutex::Mutex                    clients_mutex_;
};

}  // namespace http
}  // namespace fetch
