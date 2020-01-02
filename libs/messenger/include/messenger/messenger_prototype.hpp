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

#include "core/service_ids.hpp"
#include "messenger/mailbox.hpp"
#include "messenger/message.hpp"
#include "messenger/messenger_protocol.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"
#include "network/management/network_manager.hpp"

#include <chrono>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace messenger {

class MessengerPrototype
{
public:
  using ConstByteArray  = fetch::byte_array::ConstByteArray;
  using ResultList      = std::vector<ConstByteArray>;
  using MessageList     = MailboxInterface::MessageList;
  using NetworkManager  = fetch::network::NetworkManager;
  using ConnectionType  = std::shared_ptr<network::AbstractConnection>;
  using ServiceClient   = service::ServiceClient;
  using PublicKey       = byte_array::ConstByteArray;
  using Client          = muddle::rpc::Client;
  using Server          = muddle::rpc::Server;
  using ServerPtr       = std::shared_ptr<Server>;
  using SubscriptionPtr = muddle::MuddleEndpoint::SubscriptionPtr;
  using Endpoint        = muddle::MuddleEndpoint;
  using MuddleInterface = muddle::MuddleInterface;
  using Address         = muddle::Address;
  using Addresses       = std::unordered_set<Address>;
  using PromiseList     = std::vector<std::pair<Address, service::Promise>>;

  MessengerPrototype(muddle::MuddlePtr &muddle, Addresses node_addresses);
  MessengerPrototype(MessengerPrototype const &other) = delete;
  MessengerPrototype(MessengerPrototype &&other)      = delete;

  /// Network presence management
  /// @{
  void Register(bool require_mailbox = false);
  void Unregister();
  /// @}

  /// Mailbox management
  /// @{
  void        SendMessage(Message msg);
  void        PullMessages();
  void        ResolveMessages();
  MessageList GetMessages(uint64_t wait = 0);
  /// @}

  /// Search
  /// @{
  ResultList FindAgents(ConstByteArray const & /*type*/, ConstByteArray const & /*query*/);
  /// @}
private:
  /// Subcription handlers
  /// @{
  void OnNewMessagePacket(muddle::Packet const &packet, Address const &last_hop);
  /// @}

  /// Network components
  /// @{
  Endpoint &      endpoint_;              //< Messenger endpoint.
  Client          rpc_client_;            //< Client to perform RPC calls.
  SubscriptionPtr message_subscription_;  //< Message subscription.
  PromiseList     promises_;              //< Promises of messages for agents.
  /// @}

  /// State management
  /// @{
  MessageList inbox_;           //< Inbox of the agent
  Addresses   node_addresses_;  //< Addresses of known nodes.
  /// @}
};

}  // namespace messenger
}  // namespace fetch
