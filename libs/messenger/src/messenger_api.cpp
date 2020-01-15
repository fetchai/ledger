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
  semantic_search_module_->UnregisterAgent(call_context.sender_address);
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

QueryResult MessengerAPI::Query(service::CallContext const &call_context,
                                ConstByteArray const &query_type, ConstByteArray const &query)
{
  QueryResult ret;
  auto        agent = semantic_search_module_->GetAgent(call_context.sender_address);

  if (agent == nullptr)
  {
    ret.message = "Agent not registered";

    // Agent not registered
    return ret;
  }

  // Right now we only support semantic search
  if ((query_type != "semanticsearch") && (query_type != "semanticmodel"))
  {
    ret.message = "Unsupported search type";

    return ret;
  }

  semanticsearch::QueryCompiler compiler(ret.error_tracker, semantic_search_module_);
  auto                          compiled_query = compiler(query, "query.s");
  if (ret.error_tracker)
  {
    ret.message = "Errors during compilation";

    return ret;
  }

  // TODO: Enforce execution mode.
  semanticsearch::QueryExecutor exe(semantic_search_module_, ret.error_tracker);

  // Executing query on behalf of agent
  auto results = exe.Execute(compiled_query, agent);

  if (ret.error_tracker)
  {
    ret.message = "Errors during execution";

    return ret;
  }

  for (auto &subscription_id : *results)
  {
    auto a = semantic_search_module_->GetAgent(subscription_id);

    if (a != nullptr)
    {
      ret.agents.push_back(a->identity.identifier());
    }
  }

  return ret;
}

MessengerAPI::JSONDocument MessengerAPI::ListModels() const
{
  JSONDocument doc;

  return doc;
}

}  // namespace messenger
}  // namespace fetch
