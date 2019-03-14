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
#include "network/muddle/subscription.hpp"

#include <memory>

namespace fetch {
namespace muddle {

/**
 * The aggregation of set of subscriptions to the same service and channel combination. This is an
 * internal routing structure that performs the dispatch to all the waiting clients
 */
class SubscriptionFeed
{
public:
  using Address         = Packet::Address;
  using Payload         = Packet::Payload;
  using SubscriptionPtr = std::shared_ptr<Subscription>;

  // Construction / Destruction
  SubscriptionFeed()                         = default;
  SubscriptionFeed(SubscriptionFeed const &) = delete;
  SubscriptionFeed(SubscriptionFeed &&)      = delete;
  ~SubscriptionFeed()                        = default;

  // Operators
  SubscriptionFeed &operator=(SubscriptionFeed const &) = delete;
  SubscriptionFeed &operator=(SubscriptionFeed &&) = delete;

  SubscriptionPtr Subscribe();
  bool Dispatch(Address const &address, uint16_t service, uint16_t channel, uint16_t counter,
                Payload const &payload, Address const &transmitter);

private:
  using Mutex               = std::mutex;
  using SubscriptionWeakPtr = std::weak_ptr<Subscription>;
  using SubscriptionList    = std::vector<SubscriptionWeakPtr>;

  mutable Mutex    feed_lock_;
  SubscriptionList feed_;
};

}  // namespace muddle
}  // namespace fetch
