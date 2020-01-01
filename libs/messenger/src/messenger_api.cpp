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

#include "messenger/messenger_api.hpp"
#include "messenger/messenger_protocol.hpp"

#include <cassert>

namespace fetch {
namespace messenger {

MessengerAPI::MessengerAPI(muddle::MuddlePtr &messenger_muddle, MailboxInterface &mailbox)
  : messenger_endpoint_{messenger_muddle->GetEndpoint()}
  , messenger_protocol_{this}
  , mailbox_{mailbox}
  , advertisement_register_{std::make_shared<AdvertisementRegister>()}
  , semantic_search_module_{SemanticSearchModule::New(advertisement_register_)}
{
  rpc_server_ = std::make_shared<Server>(messenger_endpoint_, SERVICE_MESSENGER, CHANNEL_RPC);
  rpc_server_->Add(RPC_MESSENGER_INTERFACE, &messenger_protocol_);

  // Adding routing function for direct delivery
  mailbox_.SetDeliveryFunction([this](Message const &message) { AttemptDirectDelivery(message); });

  // TODO(private issue AEA-126): Move somewhere else
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
        auto            span = static_cast<uint64_t>(to - from);
        SemanticReducer cdr;
        cdr.SetReducer<Int>(1, [span, from](Int x) {
          SemanticPosition ret;
          uint64_t         multiplier = uint64_t(-1) / span;
          ret.push_back(static_cast<uint64_t>(x + from) * multiplier);

          return ret;
        });

        cdr.SetValidator<Int>([from, to](Int x) { return (from <= x) && (x <= to); });

        auto instance = DataToSubspaceMap<Int>::New();
        instance->SetSemanticReducer(cdr);

        return instance;
      });

  semantic_search_module_->RegisterFunction<ModelField, Float, Float>(
      "BoundedFloat", [](Float from, Float to) -> ModelField {
        auto            span = static_cast<Float>(to - from);
        SemanticReducer cdr;
        cdr.SetReducer<Float>(1, [span, from](Float x) {
          SemanticPosition ret;

          Float multiplier = static_cast<Float>(uint64_t(-1)) / span;
          ret.push_back(static_cast<uint64_t>((x + from) * multiplier));

          return ret;
        });

        cdr.SetValidator<Float>([from, to](Float x) { return (from <= x) && (x <= to); });

        auto instance = DataToSubspaceMap<Float>::New();
        instance->SetSemanticReducer(cdr);

        return instance;
      });

  /// TODO(private issue AEA-126): End
}

void MessengerAPI::RegisterMessenger(service::CallContext const &call_context, bool setup_mailbox)
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

void MessengerAPI::UnregisterMessenger(service::CallContext const &call_context)
{
  mailbox_.UnregisterMailbox(call_context.sender_address);
}

void MessengerAPI::SendMessage(service::CallContext const & /*call_context*/, Message msg)
{
  // TODO(private issue AEA-127): Validate sender address
  mailbox_.SendMessage(std::move(msg));
}

MessengerAPI::MessageList MessengerAPI::GetMessages(service::CallContext const &call_context)
{
  auto ret = mailbox_.GetMessages(call_context.sender_address);
  return ret;
}

void MessengerAPI::ClearMessages(service::CallContext const &call_context, uint64_t count)
{
  mailbox_.ClearMessages(call_context.sender_address, count);
}

MessengerAPI::ConstByteArray MessengerAPI::GetAddress() const
{
  return messenger_endpoint_.GetAddress();
}

MessengerAPI::ResultList MessengerAPI::FindAgents(service::CallContext const & /*call_context*/,
                                                  ConstByteArray const & /*query_type*/,
                                                  ConstByteArray const & /*query*/)
{

  return {"Hello world"};
}

void MessengerAPI::Advertise(service::CallContext const & /*call_context*/)
{}

}  // namespace messenger
}  // namespace fetch
