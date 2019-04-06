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

#include "core/mutex.hpp"
#include "network/muddle/packet.hpp"

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
      Packet::Payload const & /*payload*/, Address const & /*transmitter*/
      )>;
  using Mutex           = std::mutex;

  static constexpr char const *LOGGING_NAME = "Subscription";

  // Construction / Destruction
  Subscription()                     = default;
  Subscription(Subscription const &) = delete;
  Subscription(Subscription &&)      = delete;
  ~Subscription();

  // Operators
  Subscription &operator=(Subscription const &) = delete;
  Subscription &operator=(Subscription &&) = delete;

  void SetMessageHandler(MessageCallback const &cb);
  void Dispatch(Address const &from, uint16_t service, uint16_t channel, uint16_t counter,
                Payload const &payload, Address const &transmitter) const;

private:
  mutable Mutex   callback_lock_;
  MessageCallback callback_;
};

/**
 * Destruct the subsciption object
 */
inline Subscription::~Subscription()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Destructing subscription");

  // this is needed to ensure that no curious object
  SetMessageHandler(MessageCallback{});
}

/**
 * Sets the message handler
 *
 * @param cb  The callback method
 */
inline void Subscription::SetMessageHandler(MessageCallback const &cb)
{
  FETCH_LOCK(callback_lock_);
  callback_ = cb;
}

/**
 * Dispatch the message to the subscription
 *
 * @param service The service identifier
 * @param channel The channel identifier
 * @param payload The payload of the message
 */
inline void Subscription::Dispatch(Address const &address, uint16_t service, uint16_t channel,
                                   uint16_t counter, Payload const &payload,
                                   Address const &transmitter) const
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Dispatching subscription");

  FETCH_LOCK(callback_lock_);
  if (callback_)
  {
    callback_(address, service, channel, counter, payload, transmitter);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Dropping message because no message handler has been set");
  }
}

}  // namespace muddle
}  // namespace fetch
