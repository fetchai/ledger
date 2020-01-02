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
#include "muddle/packet.hpp"
#include "subscription_feed.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <tuple>

namespace fetch {
namespace muddle {

class Subscription;
class NetworkId;

/**
 * Top level map of subscriptions that is kept by the muddle router
 *
 * The registrar contains the top level subscription feeds, which in turn hold the list of
 * individual subscriptions. This is illustrated in the diagram below:
 *
 *  ┌────────────────┐
 *  │                │
 *  │   Registrar    │
 *  │                │
 *  └────────────────┘
 *           │
 *           │
 *           │   Service /
 *           └────Channel ──────┐
 *                 Lookup       │
 *                              │
 *                              ▼
 *                     ┌────────────────┐
 *                     │                │
 *                     │      Feed      │
 *                     │                │
 *                     └────────────────┘
 *                              │
 *                              │
 *                              │       ┌───────────────────┐    ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
 *                              │       │                   │
 *                              ├──────▶│   Subscription    │───▶│      Client       │
 *                              │       │                   │
 *                              │       └───────────────────┘    └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
 *                              │
 *                              │       ┌───────────────────┐    ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
 *                              │       │                   │
 *                              ├──────▶│   Subscription    │───▶│      Client       │
 *                              │       │                   │
 *                              │       └───────────────────┘    └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
 *                              │
 *                              │       ┌───────────────────┐    ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
 *                              │       │                   │
 *                              └──────▶│   Subscription    │───▶│      Client       │
 *                                      │                   │
 *                                      └───────────────────┘    └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
 */
class SubscriptionRegistrar
{
public:
  using SubscriptionPtr = std::shared_ptr<Subscription>;
  using PacketPtr       = std::shared_ptr<Packet>;
  using Address         = Packet::Address;

  // Construction / Destruction
  explicit SubscriptionRegistrar(NetworkId const &network);
  SubscriptionRegistrar(SubscriptionRegistrar const &) = delete;
  SubscriptionRegistrar(SubscriptionRegistrar &&)      = delete;
  ~SubscriptionRegistrar()                             = default;

  // Operators
  SubscriptionRegistrar &operator=(SubscriptionRegistrar const &) = delete;
  SubscriptionRegistrar &operator=(SubscriptionRegistrar &&) = delete;

  /// @name Subscription Registration
  /// @{
  SubscriptionPtr Register(Address const &address, uint16_t service, uint16_t channel);
  SubscriptionPtr Register(uint16_t service, uint16_t channel);
  /// @}

  bool Dispatch(PacketPtr const &packet, Address const &transmitter);

private:
  using Index              = uint32_t;
  using AddressIndex       = std::tuple<uint32_t, Address>;
  using DispatchMap        = std::map<Index, SubscriptionFeed>;
  using AddressDispatchMap = std::map<AddressIndex, SubscriptionFeed>;

  std::string const name_;
  char const *const logging_name_{name_.c_str()};

  mutable Mutex      lock_;                  ///< The registrar lock
  DispatchMap        dispatch_map_;          ///< The {service,channel} dispatch map
  AddressDispatchMap address_dispatch_map_;  ///< The {address,service,channel} dispatch map
};

}  // namespace muddle
}  // namespace fetch
