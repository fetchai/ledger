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

  MessengerAPI(muddle::MuddlePtr &messenger_muddle, MailboxInterface &mailbox);

  /// Messenger management
  /// @{
  void RegisterMessenger(service::CallContext const &call_context, bool setup_mailbox);
  void UnregisterMessenger(service::CallContext const &call_context);
  /// @}

  /// Mailbox interface
  /// @{
  void        SendMessage(service::CallContext const &call_context, Message msg);
  MessageList GetMessages(service::CallContext const &call_context);
  void        ClearMessages(service::CallContext const &call_context, uint64_t count);
  /// @}

  /// Search interface
  /// @{
  ResultList FindAgents(service::CallContext const &call_context, ConstByteArray const &query_type,
                        ConstByteArray const &query);

  void Advertise(service::CallContext const &call_context);
  /// @}

  /// Ledger interface
  /// @{
  // TODO(tfr): Yet to be written
  /// @}

  /// @{
  ConstByteArray GetAddress() const;
  /// @}

private:
  void AttemptDirectDelivery(Message const &message)
  {
    serializers::MsgPackSerializer serializer;
    serializer << message;
    messenger_endpoint_.Send(message.to.messenger, SERVICE_MESSENGER, CHANNEL_MESSENGER_MESSAGE,
                             serializer.data());
  }

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
