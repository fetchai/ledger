#pragma once

#include "agentapi/message.hpp"
#include "core/mutex.hpp"

#include "core/service_ids.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include <unordered_map>

namespace fetch {
namespace agent {

class MailboxInterface
{
public:
  using MessageList = std::vector<Message>;
  using Address     = muddle::Address;

  virtual void        SendMessage(Message message)     = 0;
  virtual MessageList GetMessages(Address agent)       = 0;
  virtual void        RegisterMailbox(Address agent)   = 0;
  virtual void        UnregisterMailbox(Address agent) = 0;
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
          message_endpoint_.Subscribe(SERVICE_MESSAGING, CHANNEL_MESSAGING_MESSAGE)}
  {}

  /// Mailbox interface
  /// @{
  void SendMessage(Message message) override
  {
    FETCH_LOCK(mutex_);

    // If the message is sent to this node, then we deliver it right
    // away
    if (message.to.node == message_endpoint_.GetAddress())
    {
      DeliverMessage(message);
      return;
    }

    // Else we pass it to the muddle for delivery
    serializers::MsgPackSerializer serializer;
    serializer << message;

    message_endpoint_.Send(message.to.node, SERVICE_MESSAGING, CHANNEL_MESSAGING_MESSAGE,
                           serializer.data());
  }

  MessageList GetMessages(Address agent) override
  {
    FETCH_LOCK(mutex_);

    // Checking that the mailbox exists
    auto it = inbox_.find(agent);
    if (it == inbox_.end())
    {
      return {};
    }

    // Emptying mailbox
    auto ret = it->second;
    it->second.clear();

    return ret;
  }
  /// @}

  /// Mailbox initialisation
  /// @{
  void RegisterMailbox(Address agent) override
  {
    FETCH_LOCK(mutex_);

    if (inbox_.find(agent) != inbox_.end())
    {
      // Mailbox already exists
      return;
    }

    // Creating an empty mailbox
    inbox_[agent] = MessageList();
  }

  void UnregisterMailbox(Address agent) override
  {
    FETCH_LOCK(mutex_);
    auto it = inbox_.find(agent);

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
  void DeliverMessage(Message const &message)
  {
    // Checking if we are delivering to the right node.
    if (message_endpoint_.GetAddress() != message.to.node)
    {
      return;
    }

    FETCH_LOCK(mutex_);
    auto it = inbox_.find(message.to.agent);
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

}  // namespace agent
}  // namespace fetch