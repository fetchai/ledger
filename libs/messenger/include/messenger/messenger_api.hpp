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

#include "core/service_ids.hpp"
#include "messenger/mailbox.hpp"
#include "messenger/message.hpp"
#include "messenger/messenger_protocol.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"
#include "semanticsearch/query/query_compiler.hpp"
#include "semanticsearch/query/query_executor.hpp"

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

  using AdvertisementRegister    = semanticsearch::AdvertisementRegister;
  using SemanticSearchModule     = semanticsearch::SemanticSearchModule;
  using AdvertisementRegisterPtr = std::shared_ptr<AdvertisementRegister>;
  using SemanticSearchModulePtr  = std::shared_ptr<semanticsearch::SemanticSearchModule>;
  using QueryExecutor            = semanticsearch::QueryExecutor;
  using SemanticReducer          = semanticsearch::SemanticReducer;
  using SemanticPosition         = semanticsearch::SemanticPosition;
  template <typename T>
  using DataToSubspaceMap = semanticsearch::DataToSubspaceMap<T>;

  MessengerAPI(muddle::MuddlePtr &messenger_muddle, MailboxInterface &mailbox)
    : messenger_endpoint_{messenger_muddle->GetEndpoint()}
    , messenger_protocol_{this}
    , mailbox_{mailbox}
    , advertisement_register_{std::make_shared<AdvertisementRegister>()}
    , semantic_search_module_{SemanticSearchModule::New(advertisement_register_)}
  {
    rpc_server_ = std::make_shared<Server>(messenger_endpoint_, SERVICE_MESSENGER, CHANNEL_RPC);
    rpc_server_->Add(RPC_MESSENGER_INTERFACE, &messenger_protocol_);

    // TODO(tfr): Move somewhere else
    using Int        = int;
    using Float      = double;
    using String     = std::string;
    using ModelField = QueryExecutor::ModelField;

    semantic_search_module_->RegisterType<Int>("Int");
    semantic_search_module_->RegisterType<Float>("Float");
    semantic_search_module_->RegisterType<String>("String");
    semantic_search_module_->RegisterType<ModelField>("ModelField", true);
    semantic_search_module_->RegisterFunction<ModelField, Int, Int>(
        "BoundedInteger", [](Int from, Int to) -> ModelField {
          uint64_t        span = static_cast<uint64_t>(to - from);
          SemanticReducer cdr;
          cdr.SetReducer<Int>(1, [span, from](Int x) {
            SemanticPosition ret;
            uint64_t         multiplier = uint64_t(-1) / span;
            ret.push_back(static_cast<uint64_t>(x + from) * multiplier);

            return ret;
          });

          cdr.SetValidator<Int>([from, to](Int x) { return (from <= x) && (x <= to); });

          auto instance = DataToSubspaceMap<Int>::New();
          instance->SetSemanticReducer(std::move(cdr));

          return instance;
        });

    semantic_search_module_->RegisterFunction<ModelField, Float, Float>(
        "BoundedFloat", [](Float from, Float to) -> ModelField {
          Float           span = static_cast<Float>(to - from);
          SemanticReducer cdr;
          cdr.SetReducer<Float>(1, [span, from](Float x) {
            SemanticPosition ret;

            Float multiplier = static_cast<Float>(uint64_t(-1)) / span;
            ret.push_back(static_cast<uint64_t>((x + from) * multiplier));

            return ret;
          });

          cdr.SetValidator<Float>([from, to](Float x) { return (from <= x) && (x <= to); });

          auto instance = DataToSubspaceMap<Float>::New();
          instance->SetSemanticReducer(std::move(cdr));

          return instance;
        });

    /// TODO(tfr): End
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

    // Adding the agent to the search register. The
    // agent first becomes searchable once it advertises
    // items on the network.
    semantic_search_module_->RegisterAgent(call_context.sender_address);
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
  ResultList FindAgents(service::CallContext const & /*call_context*/,
                        ConstByteArray /*query_type*/, ConstByteArray /*query*/)
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
  /// Networking
  /// @{
  Endpoint &        messenger_endpoint_;
  ServerPtr         rpc_server_{nullptr};
  SubscriptionPtr   message_subscription_;
  MessengerProtocol messenger_protocol_;
  /// @}

  /// Messages
  /// @{
  MailboxInterface &mailbox_;
  /// @}

  /// Advertisement and search
  /// @{
  AdvertisementRegisterPtr advertisement_register_{nullptr};
  SemanticSearchModulePtr  semantic_search_module_{nullptr};
  /// @}
};

}  // namespace messenger
}  // namespace fetch
