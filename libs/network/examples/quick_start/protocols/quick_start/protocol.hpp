#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

namespace fetch {
namespace protocols {

/* Class defining a particular protocol, that is, the calls that are
 * exposed to remotes.
 */
template <typename T>
class QuickStartProtocol : public fetch::service::Protocol
{
public:
  /* Constructor for quick start protocol attaches the functions to the protocol
   * enums
   *
   * @node is an instance of the object we want to expose the interface of
   */
  QuickStartProtocol(std::shared_ptr<T> node)
    : Protocol()
  {
    // Here we expose the functions in our class (ping, receiveMessage etc.)
    // using our protocol enums
    this->Expose(QuickStart::PING, node.get(), &T::ping);
    this->Expose(QuickStart::SEND_MESSAGE, node.get(), &T::receiveMessage);
    this->Expose(QuickStart::SEND_DATA, node.get(), &T::receiveData);
  }
};

}  // namespace protocols
}  // namespace fetch
