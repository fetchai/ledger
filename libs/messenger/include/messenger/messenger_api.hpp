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

namespace fetch {
namespace messenger {

class MessengerAPI
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using ResultList     = std::vector<ConstByteArray>;
  using NetworkManager = fetch::network::NetworkManager;
  using ConnectionType = std::shared_ptr<network::AbstractConnection>;
  using ServiceClient  = service::ServiceClient;
  using PublicKey      = byte_array::ConstByteArray;
  using MessageList    = Mailbox::MessageList;

  using Client          = muddle::rpc::Client;
  using Server          = muddle::rpc::Server;
  using ServerPtr       = std::shared_ptr<Server>;
  using SubscriptionPtr = muddle::MuddleEndpoint::SubscriptionPtr;
  using Endpoint        = muddle::MuddleEndpoint;
  using MuddleInterface = muddle::MuddleInterface;

  MessengerAPI(muddle::MuddlePtr &messenger_muddle, MailboxInterface &mailbox)
    : messenger_endpoint_{messenger_muddle->GetEndpoint()}
    , messenger_protocol_{this}
    , mailbox_{mailbox}
  {
    rpc_server_ = std::make_shared<Server>(messenger_endpoint_, SERVICE_MESSENGER, CHANNEL_RPC);
    rpc_server_->Add(RPC_MESSENGER_INTERFACE, &messenger_protocol_);
  }

  /// Messenger management
  /// @{
  void RegisterMessenger(service::CallContext const &call_context, bool setup_mailbox)
  {
    // Setting mailbox up if requested by the messenger.
    if (setup_mailbox)
    {
      mailbox_.RegisterMailbox(call_context.sender_address);
    }
  }

  void UnregisterMessenger(service::CallContext const &call_context)
  {
    mailbox_.UnregisterMailbox(call_context.sender_address);
  }
  /// @}

  /// Mailbox interface
  /// @{
  void SendMessage(service::CallContext const & /*call_context*/, Message msg)
  {
    // TODO: Validate sender address
    mailbox_.SendMessage(std::move(msg));
  }

  MessageList GetMessages(service::CallContext const &call_context)
  {
    auto ret = mailbox_.GetMessages(call_context.sender_address);
    return ret;
  }
  /// @}

  /// Search interface
  /// @{
  ResultList FindMessengers(service::CallContext const & /*call_context*/)
  {
    return {"Hello world"};
  }

  void Advertise(service::CallContext const & /*call_context*/)
  {
    std::cout << "Advertising" << std::endl;
  }
  /// @}

  /// Ledger interface
  /// @{
  // TODO: Yet to be written
  /// @}
private:
  Endpoint &        messenger_endpoint_;
  ServerPtr         rpc_server_{nullptr};
  SubscriptionPtr   message_subscription_;
  MessengerProtocol messenger_protocol_;

  MailboxInterface &mailbox_;
};

}  // namespace messenger
}  // namespace fetch