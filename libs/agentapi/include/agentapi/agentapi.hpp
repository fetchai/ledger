#pragma once
#include "agentapi/agentapi_protocol.hpp"
#include "agentapi/mailbox.hpp"
#include "agentapi/message.hpp"
#include "core/service_ids.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

namespace fetch {
namespace agent {

class AgentAPI
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

  AgentAPI(muddle::MuddlePtr &agent_muddle, MailboxInterface &mailbox)
    : agent_endpoint_{agent_muddle->GetEndpoint()}
    , agent_protocol_{this}
    , mailbox_{mailbox}
  //    , oef_comms_{new OEFComms()}
  {
    rpc_server_ = std::make_shared<Server>(agent_endpoint_, SERVICE_AGENT, CHANNEL_RPC);
    rpc_server_->Add(RPC_AGENT_INTERFACE, &agent_protocol_);
  }

  /// Agent management
  /// @{
  void RegisterAgent(service::CallContext const &call_context, bool setup_mailbox)
  {
    // Setting mailbox up if requested by the agent.
    if (setup_mailbox)
    {
      mailbox_.RegisterMailbox(call_context.sender_address);
    }
  }

  void UnregisterAgent(service::CallContext const &call_context)
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
    return mailbox_.GetMessages(call_context.sender_address);
  }
  /// @}

  /// Search interface
  /// @{
  ResultList FindAgents(service::CallContext const & /*call_context*/)
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
  Endpoint &       agent_endpoint_;
  ServerPtr        rpc_server_{nullptr};
  SubscriptionPtr  message_subscription_;
  AgentAPIProtocol agent_protocol_;

  MailboxInterface &mailbox_;
};

}  // namespace agent
}  // namespace fetch