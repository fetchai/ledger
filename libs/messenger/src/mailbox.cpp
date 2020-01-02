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

#include "logging/logging.hpp"
#include "messenger/messenger_api.hpp"
#include "messenger/messenger_protocol.hpp"

#include <cassert>

namespace fetch {
namespace messenger {

Mailbox::Mailbox(muddle::MuddlePtr &muddle)
  : message_endpoint_{muddle->GetEndpoint()}
  , message_subscription_{
        message_endpoint_.Subscribe(SERVICE_MSG_TRANSPORT, CHANNEL_MESSENGER_TRANSPORT)}
{
  message_subscription_->SetMessageHandler(this, &Mailbox::OnNewMessagePacket);
}

void Mailbox::SetDeliveryFunction(DeliveryFunction const &attempt_delivery)
{
  attempt_delivery_ = attempt_delivery;
}

void Mailbox::OnNewMessagePacket(muddle::Packet const &packet, Address const & /*last_hop*/)
{
  fetch::serializers::MsgPackSerializer serialiser(packet.GetPayload());
  try
  {
    Message message;
    serialiser >> message;
    SendMessage(message);
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_ERROR("Mailbox",
                    "Retrieved messages malformed: ", static_cast<std::string>(e.what()));
  }
}

void Mailbox::SendMessage(Message message)
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

  message_endpoint_.Send(message.to.node, SERVICE_MSG_TRANSPORT, CHANNEL_MESSENGER_TRANSPORT,
                         serializer.data());
}

Mailbox::MessageList Mailbox::GetMessages(Address messenger)
{
  FETCH_LOCK(mutex_);

  // Checking that the mailbox exists
  auto it = inboxes_.find(messenger);
  if (it == inboxes_.end())
  {
    return {};
  }

  return it->second;
}

void Mailbox::ClearMessages(Address messenger, uint64_t count)
{
  FETCH_LOCK(mutex_);

  // Checking that the mailbox exists
  auto it = inboxes_.find(messenger);
  if (it == inboxes_.end())
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

void Mailbox::RegisterMailbox(Address messenger)
{
  FETCH_LOCK(mutex_);

  if (inboxes_.find(messenger) != inboxes_.end())
  {
    // Mailbox already exists
    return;
  }

  // Creating an empty mailbox
  inboxes_[messenger] = MessageList();
}

void Mailbox::UnregisterMailbox(Address messenger)
{
  FETCH_LOCK(mutex_);
  auto it = inboxes_.find(messenger);

  // Checking if mailbox exists
  if (it == inboxes_.end())
  {
    return;
  }

  // Deleting mailbox
  inboxes_.erase(it);
}

void Mailbox::DeliverMessageLockLess(Message const &message)
{
  // Checking if we are delivering to the right node.
  if (message_endpoint_.GetAddress() != message.to.node)
  {
    return;
  }

  auto it = inboxes_.find(message.to.messenger);
  if (it == inboxes_.end())
  {
    // Attempting to deliver directly
    if (attempt_delivery_)
    {
      attempt_delivery_(message);
    }

    return;
  }

  // Adding message to mailbox
  it->second.push_back(message);
}

}  // namespace messenger
}  // namespace fetch
