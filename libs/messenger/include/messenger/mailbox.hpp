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

#include "core/mutex.hpp"
#include "core/service_ids.hpp"
#include "messenger/mailbox_interface.hpp"
#include "messenger/message.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include <deque>
#include <functional>
#include <unordered_map>

namespace fetch {
namespace messenger {

class Mailbox final : public MailboxInterface
{
public:
  using Address         = muddle::Address;
  using Endpoint        = muddle::MuddleEndpoint;
  using MuddleInterface = muddle::MuddleInterface;
  using SubscriptionPtr = muddle::MuddleEndpoint::SubscriptionPtr;

  explicit Mailbox(muddle::MuddlePtr &muddle);
  ~Mailbox() override = default;

  void SetDeliveryFunction(DeliveryFunction const &attempt_delivery) override;

  /// Mailbox interface
  /// @{
  void        SendMessage(Message message) override;
  MessageList GetMessages(Address messenger) override;
  void        ClearMessages(Address messenger, uint64_t count) override;
  /// @}

  /// Mailbox initialisation
  /// @{
  void RegisterMailbox(Address messenger) override;
  void UnregisterMailbox(Address messenger) override;
  /// @}

protected:
  void DeliverMessageLockLess(Message const &message);

  /// Subscription Hnadlers
  /// @{
  void OnNewMessagePacket(muddle::Packet const &packet, Address const &last_hop);
  /// }

  // TODO(private issue AEA-122): Add state logic to trim inboxes
  //
private:
  Mutex                                    mutex_{};
  std::unordered_map<Address, MessageList> inboxes_{};

  Endpoint &      message_endpoint_;
  SubscriptionPtr message_subscription_;

  DeliveryFunction attempt_delivery_;
};

}  // namespace messenger
}  // namespace fetch
