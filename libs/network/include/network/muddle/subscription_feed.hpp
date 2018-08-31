#ifndef FETCH_SUBSCRIPTION_FEED_HPP
#define FETCH_SUBSCRIPTION_FEED_HPP

#include "core/mutex.hpp"
#include "network/muddle/subscription.hpp"

#include <memory>

namespace fetch {
namespace muddle {

/**
 * The aggregation of the subscription
 */
class SubscriptionFeed
{
public:
  using Address = Packet::Address;
  using Payload = Packet::Payload;
  using SubscriptionPtr = std::shared_ptr<Subscription>;

  // Construction / Destruction
  SubscriptionFeed() = default;
  SubscriptionFeed(SubscriptionFeed const &) = delete;
  SubscriptionFeed(SubscriptionFeed &&) = delete;
  ~SubscriptionFeed() = default;

  // Operators
  SubscriptionFeed &operator=(SubscriptionFeed const &) = delete;
  SubscriptionFeed &operator=(SubscriptionFeed &&) = delete;

  SubscriptionPtr Subscribe();
  bool Dispatch(Address const &address, uint16_t service, uint16_t channel, uint16_t counter, Payload const &payload);

private:

  using Mutex = mutex::Mutex;
  using SubscriptionWeakPtr = std::weak_ptr<Subscription>;
  using SubscriptionList = std::vector<SubscriptionWeakPtr>;

  mutable Mutex    feed_lock_{__LINE__, __FILE__};
  SubscriptionList feed_;
};

} // namespace muddle
} // namespace fetch

#endif //FETCH_SUBSCRIPTION_FEED_HPP
