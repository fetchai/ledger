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

#include "messenger/messenger_prototype.hpp"

#include "logging/logging.hpp"

#include <cassert>

namespace fetch {
namespace messenger {

MessengerPrototype::MessengerPrototype(muddle::MuddlePtr &muddle, Addresses node_addresses)
  : endpoint_{muddle->GetEndpoint()}
  , rpc_client_{"Messenger", endpoint_, SERVICE_MESSENGER, CHANNEL_RPC}
  , message_subscription_{endpoint_.Subscribe(SERVICE_MESSENGER, CHANNEL_MESSENGER_MESSAGE)}
  , node_addresses_{std::move(node_addresses)}
{
  message_subscription_->SetMessageHandler(this, &MessengerPrototype::OnNewMessagePacket);
}

void MessengerPrototype::Register(bool require_mailbox)
{
  // Registering with all connected nodes
  for (auto &address : node_addresses_)
  {
    rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                    MessengerProtocol::REGISTER_MESSENGER, require_mailbox);
  }
}

void MessengerPrototype::Unregister()
{
  for (auto &address : node_addresses_)
  {
    rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                    MessengerProtocol::UNREGISTER_MESSENGER);
  }
}

void MessengerPrototype::SendMessage(Message msg)
{
  // Checking if any node connectivity exists
  if (node_addresses_.empty())
  {
    throw std::runtime_error("Not connected to any nodes.");
  }

  msg.from.messenger = endpoint_.GetAddress();

  // Checking if we are directly connected to the relevant
  // node
  for (auto &address : node_addresses_)
  {
    if (msg.to.node == address)
    {
      rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                      MessengerProtocol::SEND_MESSAGE, msg);
      return;
    }
  }

  // Sending to the first node in the list
  auto address = *node_addresses_.begin();
  if (msg.from.node.empty())
  {
    msg.from.node = address;
  }

  rpc_client_.CallSpecificAddress(msg.from.node, RPC_MESSENGER_INTERFACE,
                                  MessengerProtocol::SEND_MESSAGE, msg);
}

void MessengerPrototype::PullMessages()
{
  // Sending request for messages and storing promises
  // such they can be realised later.
  for (auto &address : node_addresses_)
  {
    auto promise = rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                                   MessengerProtocol::GET_MESSAGES);
    promises_.push_back({address, promise});
  }
}

void MessengerPrototype::ResolveMessages()
{
  // List to schedule unresolved promised for later
  PromiseList unresolved{};

  // Realising promis one by one
  for (auto &x : promises_)
  {
    auto &address = x.first;
    auto &p       = x.second;

    switch (p->state())
    {
    case service::PromiseState::WAITING:
      // Those which have not got a response yet
      // are kept for later
      unresolved.push_back(x);
      break;
    case service::PromiseState::SUCCESS:
    {
      MessageList list{};
      if (!p->GetResult(list))
      {
        break;
      }

      // Adding the resulting messages to the inbox
      for (auto const &msg : list)
      {
        inbox_.push_back(msg);
      }

      // Removing messages from the mailbox
      rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                      MessengerProtocol::CLEAR_MESSAGES,
                                      static_cast<uint64_t>(list.size()));
      break;
    }
    case service::PromiseState::FAILED:
    case service::PromiseState::TIMEDOUT:
      // Ignored as they will not contain any data.
      break;
    }
  }

  // Storing unresolved promises for later.
  promises_ = unresolved;
}

MessengerPrototype::MessageList MessengerPrototype::GetMessages(uint64_t wait)
{
  // Sending pull requests for messages
  PullMessages();

  // Adding delay to allow for result to arrive back
  std::this_thread::sleep_for(std::chrono::milliseconds(wait));

  // Realising promises
  ResolveMessages();

  // Creating return value and emptying messages.
  MessageList ret;
  std::swap(ret, inbox_);

  // Turning all pulled requests into messages
  PromiseList remaining;
  for (auto &x : promises_)
  {
    auto &p = x.second;

    if (p->IsWaiting())
    {
      remaining.push_back(x);
    }
    else
    {
      // Ignoring failed promises
      if (p->IsFailed())
      {
        continue;
      }

      // Adding results to return value
      MessageList messages{};
      if (p->GetResult(messages))
      {
        for (auto &m : messages)
        {
          ret.push_back(m);
        }
      }
    }
  }

  // Storing the remaining promises to be
  // resolved later.
  std::swap(remaining, promises_);

  return ret;
}

void MessengerPrototype::OnNewMessagePacket(muddle::Packet const &packet,
                                            Address const & /*last_hop*/)
{
  fetch::serializers::MsgPackSerializer serialiser(packet.GetPayload());

  try
  {
    Message message;
    serialiser >> message;
    inbox_.push_back(message);
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_ERROR("MessengerPrototype",
                    "Retrieved messages malformed: ", static_cast<std::string>(e.what()));
  }
}

MessengerPrototype::ResultList MessengerPrototype::FindAgents(ConstByteArray const & /*type*/,
                                                              ConstByteArray const & /*query*/)
{
  return {};
}

}  // namespace messenger
}  // namespace fetch
