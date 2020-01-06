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
#include "logging/logging.hpp"
#include "muddle/packet.hpp"

#include <cstdint>
#include <functional>

namespace fetch {
namespace muddle {

/**
 * Subscription is an object that wraps callbacks to a given client for messages. These objects are
 * help by both client and inside the router for the purpose of message dispatching
 */
class Subscription
{
public:
  using Address         = Packet::Address;
  using Payload         = Packet::Payload;
  using Handle          = uint64_t;
  using MessageCallback = std::function<void(
      Address const & /*from*/, uint16_t /*service*/, uint16_t /*channel*/, uint16_t /*counter*/,
      Payload const & /*payload*/, Address const & /*last hop*/
      )>;
  using LowLevelCallback =
      std::function<void(Packet const & /*packet*/, Address const & /*last hop*/)>;
  using BasicMessageCallback =
      std::function<void(Address const & /*address*/, Payload const & /*payload*/)>;

  static constexpr char const *LOGGING_NAME = "Subscription";

  // Construction / Destruction
  Subscription()                     = default;
  Subscription(Subscription const &) = delete;
  Subscription(Subscription &&)      = delete;
  ~Subscription();

  void SetMessageHandler(MessageCallback cb);
  void SetMessageHandler(BasicMessageCallback cb);
  void SetMessageHandler(LowLevelCallback cb);

  template <typename Class>
  void SetMessageHandler(Class *instance,
                         void (Class::*member_function)(Address const &, Packet::Payload const &));

  template <typename Class>
  void SetMessageHandler(Class *instance,
                         void (Class::*member_function)(Packet const &, Address const &));

  void Dispatch(Packet const &packet, Address const &last_hop) const;

  // Operators
  Subscription &operator=(Subscription const &) = delete;
  Subscription &operator=(Subscription &&) = delete;

private:
  mutable Mutex    callback_lock_;
  LowLevelCallback callback_;
};

template <typename Class>
void Subscription::SetMessageHandler(Class *instance,
                                     void (Class::*member_function)(Address const &,
                                                                    Packet::Payload const &))
{
  SetMessageHandler(
      [instance, member_function](Address const &address, Packet::Payload const &payload) {
        (instance->*member_function)(address, payload);
      });
}

template <typename Class>
void Subscription::SetMessageHandler(Class *instance,
                                     void (Class::*member_function)(Packet const &,
                                                                    Address const &))
{
  SetMessageHandler([instance, member_function](Packet const &pkt, Address const &last_hop) {
    (instance->*member_function)(pkt, last_hop);
  });
}

}  // namespace muddle
}  // namespace fetch
