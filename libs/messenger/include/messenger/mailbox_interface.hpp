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

#include "messenger/message.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"

#include <deque>
#include <unordered_map>

namespace fetch {
namespace messenger {

class MailboxInterface
{
public:
  using MessageList      = std::deque<Message>;
  using Address          = muddle::Address;
  using DeliveryFunction = std::function<void(Message const &msg)>;

  virtual ~MailboxInterface() = default;

  virtual void        SetDeliveryFunction(DeliveryFunction const &attempt_delivery) = 0;
  virtual void        SendMessage(Message message)                                  = 0;
  virtual MessageList GetMessages(Address messenger)                                = 0;
  virtual void        ClearMessages(Address messenger, uint64_t count)              = 0;
  virtual void        RegisterMailbox(Address messenger)                            = 0;
  virtual void        UnregisterMailbox(Address messenger)                          = 0;
};

}  // namespace messenger
}  // namespace fetch
