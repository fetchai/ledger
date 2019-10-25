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

#include "messenger/messenger_prototype.hpp"

#include <cassert>

namespace fetch {
namespace messenger {

MessengerPrototype::MessengerPrototype(muddle::MuddlePtr &muddle, Addresses node_addresses)
  : endpoint_{muddle->GetEndpoint()}
  , rpc_client_{"Messenger", endpoint_, SERVICE_MESSENGER, CHANNEL_RPC}
  , message_subscription_{endpoint_.Subscribe(SERVICE_MESSENGER, CHANNEL_MESSENGER_MESSAGE)}
  , node_addresses_{std::move(node_addresses)}
{
  //    rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_MESSENGER, CHANNEL_RPC);
  //    rpc_server_->Add(RPC_NODE_TO_MESSENGER, &protocol_);
}

void MessengerPrototype::Register(bool require_mailbox)
{
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

void MessengerPrototype::SendMessage(Message const &msg)
{
  // Checking if any node connectivity exists
  if (node_addresses_.size() == 0)
  {
    throw std::runtime_error("Not connected to any nodes.");
  }

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
  rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE, MessengerProtocol::SEND_MESSAGE,
                                  msg);
}

void MessengerPrototype::PullMessages()
{
  // Sending request for messages and storing promises
  // such they can be realised later.
  for (auto &address : node_addresses_)
  {
    auto promise = rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                                   MessengerProtocol::GET_MESSAGES);
    promises_.push_back(promise);
  }
}

void MessengerPrototype::ResolveMessages()
{
  // List to schedule unresolved promised for later
  PromiseList unresolved{};

  // Realising promis one by one
  for (auto &p : promises_)
  {
    switch (p->state())
    {
    case service::PromiseState::WAITING:
      // Those which have not got a response yet
      // are kept for later
      unresolved.push_back(p);
      break;
    case service::PromiseState::SUCCESS:
      // Adding the resulting messages to the inbox
      for (auto const &msg : p->As<MessageList>())
      {
        inbox_.push_back(msg);
      }
      break;
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
  auto ret = inbox_;
  inbox_.clear();

  // Turning all pulled requests into messages
  PromiseList remaining;
  for (auto &p : promises_)
  {
    if (p->IsWaiting())
    {
      remaining.push_back(p);
    }
    else
    {
      // Ignoring failed promises
      if (p->IsFailed())
      {
        continue;
      }

      // Adding results to return value
      auto messages = p->As<MessageList>();
      for (auto &m : messages)
      {
        ret.push_back(m);
      }
    }
  }

  // Storing the remaining promises to be
  // resolved later.
  std::swap(remaining, promises_);

  return ret;
}

MessengerPrototype::ResultList MessengerPrototype::FindAgents(ConstByteArray /*type*/,
                                                              ConstByteArray /*query*/)
{
  return {};
}

}  // namespace messenger
}  // namespace fetch
