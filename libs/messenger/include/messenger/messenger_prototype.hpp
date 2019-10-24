#pragma once

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
  using PromiseList     = std::vector<service::Promise>;
  using Client          = muddle::rpc::Client;
  using Server          = muddle::rpc::Server;
  using ServerPtr       = std::shared_ptr<Server>;
  using SubscriptionPtr = muddle::MuddleEndpoint::SubscriptionPtr;
  using Endpoint        = muddle::MuddleEndpoint;
  using MuddleInterface = muddle::MuddleInterface;
  using Address         = muddle::Address;
  using Addresses       = std::unordered_set<Address>;

  Endpoint &      endpoint_;
  Client          rpc_client_;  // TODO: Move
  SubscriptionPtr message_subscription_;

  MessengerPrototype(muddle::MuddlePtr &muddle, Addresses node_addresses)
    : endpoint_{muddle->GetEndpoint()}
    , rpc_client_{"Messenger", endpoint_, SERVICE_MESSENGER, CHANNEL_RPC}
    , message_subscription_{endpoint_.Subscribe(SERVICE_MESSENGER, CHANNEL_MESSENGER_MESSAGE)}
    , node_addresses_{std::move(node_addresses)}
  {
    //    rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_MESSENGER, CHANNEL_RPC);
    //    rpc_server_->Add(RPC_NODE_TO_MESSENGER, &protocol_);
  }

  /// @{
  void Register(bool require_mailbox = false)
  {
    for (auto &address : node_addresses_)
    {
      rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                      MessengerProtocol::REGISTER_MESSENGER, require_mailbox);
    }
  }

  void Unregister()
  {
    for (auto &address : node_addresses_)
    {
      rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                      MessengerProtocol::UNREGISTER_MESSENGER);
    }
  }
  /// @}

  /// Mailbox management
  /// @{
  void SendMessage(Message const &msg)
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
    rpc_client_.CallSpecificAddress(address, RPC_MESSENGER_INTERFACE,
                                    MessengerProtocol::SEND_MESSAGE, msg);
  }

  void PullMessages()
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

  void ResolveMessages()
  {
    PromiseList unresolved{};
    for (auto &p : promises_)
    {
      switch (p->state())
      {
      case service::PromiseState::WAITING:
        unresolved.push_back(p);
        break;
      case service::PromiseState::SUCCESS:
        for (auto const &msg : p->As<MessageList>())
        {
          inbox_.push_back(msg);
        }
        break;
      case service::PromiseState::FAILED:
      case service::PromiseState::TIMEDOUT:
        break;
      }
    }

    promises_ = unresolved;
  }

  MessageList GetMessages(uint64_t wait = 0)
  {
    // Sending pull requests for messages
    PullMessages();

    if (wait != 0ull)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }

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
  /// @}

  /// Search
  /// @{
  ResultList FindMessengers(ConstByteArray query)
  {
    return {};
  }
  /// @}
private:
  MessageList inbox_;
  Addresses   node_addresses_;
  PromiseList promises_;
};

}  // namespace messenger
}  // namespace fetch