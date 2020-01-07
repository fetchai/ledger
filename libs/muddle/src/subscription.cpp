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

#include "muddle/subscription.hpp"

namespace fetch {
namespace muddle {

/**
 * Destruct the subscription object
 */
Subscription::~Subscription()
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
void Subscription::SetMessageHandler(MessageCallback cb)
{
  SetMessageHandler([c = std::move(cb)](Packet const &pkt, Address const &last_hop) {
    c(pkt.GetSender(), pkt.GetService(), pkt.GetChannel(), pkt.GetMessageNum(), pkt.GetPayload(),
      last_hop);
  });
}

void Subscription::SetMessageHandler(BasicMessageCallback cb)
{
  SetMessageHandler([c = std::move(cb)](Packet const &pkt, Address const &last_hop) {
    FETCH_UNUSED(last_hop);
    c(pkt.GetSender(), pkt.GetPayload());
  });
}

void Subscription::SetMessageHandler(LowLevelCallback cb)
{
  FETCH_LOCK(callback_lock_);
  callback_ = std::move(cb);
}

/**
 * Dispatch the message to the subscription
 *
 * @param service The service identifier
 * @param channel The channel identifier
 * @param payload The payload of the message
 */
void Subscription::Dispatch(Packet const &packet, Address const &last_hop) const
{
  FETCH_LOCK(callback_lock_);
  if (callback_)
  {
    callback_(packet, last_hop);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Dropping message because no message handler has been set");
  }
}

}  // namespace muddle
}  // namespace fetch
