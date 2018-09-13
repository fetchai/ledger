#ifndef FETCH_SUBSCRIPTION_HPP
#define FETCH_SUBSCRIPTION_HPP

#include "core/mutex.hpp"
#include "network/muddle/packet.hpp"

#include <cstdint>
#include <functional>

namespace fetch {
namespace muddle {

/**
 * Subscription object
 */
class Subscription
{
public:
  using Address = Packet::Address;
  using Payload = Packet::Payload;
  using Handle = uint64_t;
  using MessageCallback = std::function<
    void(Address const &,uint16_t, uint16_t, uint16_t, Packet::Payload const &)
  >;
  using Mutex = mutex::Mutex;

  static constexpr char const *LOGGING_NAME = "Subscription";

  // Construction / Destruction
  Subscription() = default;
  Subscription(Subscription const &) = delete;
  Subscription(Subscription &&) = delete;
  ~Subscription();

  // Operators
  Subscription &operator=(Subscription const &) = delete;
  Subscription &operator=(Subscription &&) = delete;

  void SetMessageHandler(MessageCallback const &cb);
  void Dispatch(Address const &from,
                uint16_t service,
                uint16_t channel,
                uint16_t counter,
                Payload const &payload) const;

private:

  mutable Mutex callback_lock_{__LINE__, __FILE__};
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
inline void Subscription::Dispatch(Address const &address, uint16_t service, uint16_t channel, uint16_t counter, Payload const &payload) const
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Dispatching subscription");

  FETCH_LOCK(callback_lock_);
  if (callback_)
  {
    callback_(address, service, channel, counter, payload);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Dropping message because no message handler has been set");
  }
}

} // namespace muddle
} // namespace fetch

#endif //FETCH_SUBSCRIPTION_HPP
