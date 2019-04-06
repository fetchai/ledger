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

#include "network/service/protocol.hpp"

#include "commands.hpp"
#include "node.hpp"

namespace fetch {
namespace protocols {

/* Class defining a particular protocol, that is, the calls that are
 * exposed to remotes. In this instance we are allowing interested parties to
 * register to listen to 'new message's from the Node
 */
class SubscribeProtocol : public fetch::service::Protocol, public subscribe::Node
{
public:
  /*
   * Constructor for subscribe protocol attaches the functions to the protocol
   * enums
   */
  SubscribeProtocol()
    : Protocol()
    , Node()
  {
    // We are allowing 'this' Node to register the feed new_message on the
    // protocol
    this->RegisterFeed(SubscribeProto::NEW_MESSAGE, this);
  }
};

}  // namespace protocols
}  // namespace fetch
