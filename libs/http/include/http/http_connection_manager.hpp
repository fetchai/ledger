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

#include "core/byte_array/byte_array.hpp"
#include "http/abstract_connection.hpp"
#include "http/abstract_server.hpp"
#include "logging/logging.hpp"

#include <cstdint>

namespace fetch {
namespace http {

class HTTPConnectionManager
{
public:
  using ConnectionType = typename AbstractHTTPConnection::SharedType;
  using HandleType     = uint64_t;

  static constexpr char const *LOGGING_NAME = "HTTPConnectionManager";

  explicit HTTPConnectionManager(AbstractHTTPServer &server);
  ~HTTPConnectionManager();

  HandleType  Join(ConnectionType client);
  void        Leave(HandleType handle);
  bool        Send(HandleType client, HTTPResponse const &res);
  void        PushRequest(HandleType client, HTTPRequest const &req);
  std::string GetAddress(HandleType client);

  bool has_connections() const;

private:
  AbstractHTTPServer &                 server_;
  std::map<HandleType, ConnectionType> clients_;
  mutable Mutex                        clients_mutex_;
};

}  // namespace http
}  // namespace fetch
