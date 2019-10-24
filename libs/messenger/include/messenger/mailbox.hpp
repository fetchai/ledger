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

#include "core/mutex.hpp"
#include "messenger/message.hpp"

#include "core/service_ids.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include <deque>
#include <unordered_map>

namespace fetch {
namespace messenger {

class MailboxInterface
{
public:
  using MessageList = std::deque<Message>;
  using Address     = muddle::Address;

  virtual void        SendMessage(Message message)                     = 0;
  virtual MessageList GetMessages(Address messenger)                   = 0;
  virtual void        ClearMessages(Address messenger, uint64_t count) = 0;
  virtual void        RegisterMailbox(Address messenger)               = 0;
  virtual void        UnregisterMailbox(Address messenger)             = 0;
};

class Mailbox : public MailboxInterface
{
public:
  using Address         = muddle::Address;
  using Endpoint        = muddle::MuddleEndpoint;
  using MuddleInterface = muddle::MuddleInterface;
  using SubscriptionPtr = muddle::MuddleEndpoint::SubscriptionPtr;

  Mailbox(muddle::MuddlePtr &muddle)
    : message_endpoint_{muddle->GetEndpoint()}
    , message_subscription_{
          message_endpoint_.Subscribe(SERVICE_MSG_TRANSPORT, CHANNEL_MESSEGNER_TRANSPORT)}
  {}

  /// Mailbox interface
  /// @{
  void SendMessage(Message message) override
  {
    FETCH_LOCK(mutex_);

    // Setting the entry node
    if (message.from.node.empty())
    {
      message.from.node = message_endpoint_.GetAddress();
    }

    // Setting to node
    if (message.to.node.empty())
    {
      message.to.node = message_endpoint_.GetAddress();
    }

    // If the message is sent to this node, then we deliver it right
    // away
    if (message.to.node == message_endpoint_.GetAddress())
    {
      DeliverMessageLockLess(message);
      return;
    }

    // Else we pass it to the muddle for delivery
    serializers::MsgPackSerializer serializer;
    serializer << message;

    message_endpoint_.Send(message.to.node, SERVICE_MSG_TRANSPORT, CHANNEL_MESSEGNER_TRANSPORT,
                           serializer.data());
  }

  MessageList GetMessages(Address messenger) override
  {
    FETCH_LOCK(mutex_);

    // Checking that the mailbox exists
    auto it = inbox_.find(messenger);
    if (it == inbox_.end())
    {
      return {};
    }

    return it->second;
  }

  void ClearMessages(Address messenger, uint64_t count) override
  {
    FETCH_LOCK(mutex_);

    // Checking that the mailbox exists
    auto it = inbox_.find(messenger);
    if (it == inbox_.end())
    {
      return;
    }

    // Emptying mailbox
    if (count >= it->second.size())
    {
      it->second.clear();
    }
    else
    {
      while (count != 0)
      {
        it->second.pop_front();
        --count;
      }
    }
  }
  /// @}

  /// Mailbox initialisation
  /// @{
  void RegisterMailbox(Address messenger) override
  {
    FETCH_LOCK(mutex_);

    if (inbox_.find(messenger) != inbox_.end())
    {
      // Mailbox already exists
      return;
    }

    // Creating an empty mailbox
    inbox_[messenger] = MessageList();
  }

  void UnregisterMailbox(Address messenger) override
  {
    FETCH_LOCK(mutex_);
    auto it = inbox_.find(messenger);

    // Checking if mailbox exists
    if (it == inbox_.end())
    {
      return;
    }

    // Deleting mailbox
    inbox_.erase(it);
  }
  /// @}

protected:
  void DeliverMessageLockLess(Message const &message)
  {
    // Checking if we are delivering to the right node.
    if (message_endpoint_.GetAddress() != message.to.node)
    {
      return;
    }

    auto it = inbox_.find(message.to.messenger);
    if (it == inbox_.end())
    {
      // Attempting to deliver directly
      // TODO: Attempt direct delivery
      return;
    }

    // Adding message to mailbox
    it->second.push_back(message);
  }

  // TODO: Add state logic to trim inboxes
  //

private:
  Mutex                                    mutex_{};
  std::unordered_map<Address, MessageList> inbox_{};

  Endpoint &      message_endpoint_;
  SubscriptionPtr message_subscription_;
};

}  // namespace messenger
}  // namespace fetch
