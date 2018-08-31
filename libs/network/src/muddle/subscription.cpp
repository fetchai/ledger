#include "network/muddle/subscription.hpp"

namespace fetch {
namespace muddle {

Subscription::Mutex Subscription::counter_lock_{__LINE__, __FILE__};
Subscription::Handle Subscription::counter_{0};

Subscription::Handle Subscription::GetNextCounter()
{
  FETCH_LOCK(counter_lock_);
  return counter_++;
}

} // namespace muddle
} // namespace fetch
