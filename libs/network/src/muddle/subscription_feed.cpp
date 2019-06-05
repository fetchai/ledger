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

#include "network/muddle/subscription_feed.hpp"

#include "core/mutex.hpp"

namespace fetch {
namespace muddle {

/**
 * Create a new subscription from this feed
 *
 * @return The new subscription to the feed
 */
SubscriptionFeed::SubscriptionPtr SubscriptionFeed::Subscribe()
{
  auto subscription = std::make_shared<Subscription>();

  {
    FETCH_LOCK(feed_lock_);
    feed_.push_back(subscription);
  }

  return subscription;
}

/**
 * Dispatch the contents of the message
 *
 * @param service The service identifier
 * @param channel The channel identifier
 * @param counter The message number  / identifier
 * @param payload The payload of the message
 * @return true if one or more successful dispatches were made, otherwise false
 */
bool SubscriptionFeed::Dispatch(Address const &address, uint16_t service, uint16_t channel,
                                uint16_t counter, Payload const &payload,
                                Address const &transmitter)
{
  FETCH_LOCK(feed_lock_);

  bool success = false;

  // loop through the subscriptions
  auto it = feed_.cbegin();
  while (it != feed_.cend())
  {
    // check
    auto subscription = it->lock();
    if (subscription)
    {
      // dispatch the message to the handler
      subscription->Dispatch(address, service, channel, counter, payload, transmitter);

      success = true;
      ++it;
    }
    else
    {
      // if the subscription is dead then remove it from our list
      it = feed_.erase(it);
    }
  }

  return success;
}

}  // namespace muddle
}  // namespace fetch
