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

#include "network/management/abstract_connection.hpp"
#include "network/message.hpp"

namespace fetch {
namespace network {

class AbstractNetworkServer
{
public:
  using ConnectionHandleType = typename AbstractConnection::ConnectionHandleType;

  // Construction / Destruction
  AbstractNetworkServer()          = default;
  virtual ~AbstractNetworkServer() = default;

  /// @name Abstract Network Server Interface
  /// @{
  virtual uint16_t GetListeningPort() const                                           = 0;
  virtual void     PushRequest(ConnectionHandleType client, MessageBuffer const &msg) = 0;
  /// @}
};

}  // namespace network
}  // namespace fetch
