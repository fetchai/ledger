#ifndef FETCH_SUBSCRIPTION_MANAGER_HPP
#define FETCH_SUBSCRIPTION_MANAGER_HPP

#include "core/mutex.hpp"
#include "network/muddle/subscription.hpp"
#include "network/muddle/subscription_feed.hpp"
#include "network/muddle/packet.hpp"

#include <tuple>
#include <map>

namespace fetch {
namespace muddle {

/**
 *
 */
class SubscriptionRegistrar
{
public:
  using SubscriptionPtr = std::shared_ptr<Subscription>;
  using PacketPtr       = std::shared_ptr<Packet>;
  using Address         = Packet::Address;

  static constexpr char const *LOGGING_NAME = "SubscriptionRegistrar";

  // Construction / Destruction
  SubscriptionRegistrar() = default;
  SubscriptionRegistrar(SubscriptionRegistrar const &) = delete;
  SubscriptionRegistrar(SubscriptionRegistrar &&) = delete;
  ~SubscriptionRegistrar() = default;

  // Operators
  SubscriptionRegistrar &operator=(SubscriptionRegistrar const &) = delete;
  SubscriptionRegistrar &operator=(SubscriptionRegistrar &&) = delete;

  /// @name Subscription Registration
  /// @{
  SubscriptionPtr Register(Address const &address, uint16_t service, uint16_t channel);
  SubscriptionPtr Register(uint16_t service, uint16_t channel);
  /// @}

  bool Dispatch(PacketPtr packet);

private:
  using Mutex              = mutex::Mutex;
  using Index              = uint32_t;
  using AddressIndex       = std::tuple<uint32_t, Address>;
  using DispatchMap        = std::map<Index, SubscriptionFeed>;
  using AddressDispatchMap = std::map<AddressIndex, SubscriptionFeed>;

  mutable Mutex       lock_{__LINE__, __FILE__};  ///< The registrar lock
  DispatchMap         dispatch_map_;              ///< The {service,channel} dispatch map
  AddressDispatchMap  address_dispatch_map_;      ///< The {address,service,channel} dispatch map
};

} // namespace muddle
} // namespace fetch

#endif //FETCH_SUBSCRIPTION_MANAGER_HPP
