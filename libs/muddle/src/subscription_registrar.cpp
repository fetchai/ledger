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

#include "muddle_logging_name.hpp"
#include "subscription_registrar.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "logging/logging.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace fetch {
namespace muddle {
namespace {

constexpr char const *BASE_NAME = "SubRegister";

/**
 * Combine the service and channel identifiers
 *
 * @param service The service identifier
 * @param channel The channel identifier
 * @return
 */
uint32_t Combine(uint16_t service, uint16_t channel)
{
  uint32_t index = 0;
  index |= static_cast<uint32_t>(service) << 16u;
  index |= static_cast<uint32_t>(channel);
  return index;
}

}  // namespace

SubscriptionRegistrar::SubscriptionRegistrar(NetworkId const &network)
  : name_{GenerateLoggingName(BASE_NAME, network)}
{}

/**
 * Register a subscription with an address, service and channel identifier
 *
 * @param address The target address
 * @param service The target service
 * @param channel The target channel
 * @return A valid subscription pointer if successful, otherwise an invalid pointer
 */
SubscriptionRegistrar::SubscriptionPtr SubscriptionRegistrar::Register(Address const &address,
                                                                       uint16_t       service,
                                                                       uint16_t       channel)
{
  SubscriptionPtr subscription;

  AddressIndex const index{Combine(service, channel), address};

  {
    FETCH_LOCK(lock_);

    auto &feed   = address_dispatch_map_[index];
    subscription = feed.Subscribe();
  }

  return subscription;
}

/**
 * Register a subscription with a service and channel identifier
 *
 * @param service The target service
 * @param channel The target channel
 * @return A valid subscription pointer if successful, otherwise an invalid pointer
 */
SubscriptionRegistrar::SubscriptionPtr SubscriptionRegistrar::Register(uint16_t service,
                                                                       uint16_t channel)
{
  SubscriptionPtr subscription;

  Index const index = Combine(service, channel);

  {
    FETCH_LOCK(lock_);

    auto &feed   = dispatch_map_[index];
    subscription = feed.Subscribe();
  }

  return subscription;
}

/**
 * Dispatch the packet to subscriptions
 *
 * @param packet The packet
 * @return true if successfully dispatched, otherwise false
 */
bool SubscriptionRegistrar::Dispatch(PacketPtr const &packet, Address const &transmitter)
{
  bool success = false;

  Index const  index = Combine(packet->GetService(), packet->GetChannel());
  AddressIndex address_index{index, packet->GetTarget()};

  {
    FETCH_LOCK(lock_);
    auto it = dispatch_map_.find(index);
    if (it != dispatch_map_.end())
    {
      // dispatch the packet to the subscription feed
      success = it->second.Dispatch(*packet, transmitter);

      if (!success)
      {
        FETCH_LOG_WARN(logging_name_, "Failed to dispatch message to a given subscription");
      }
    }
  }

  {
    FETCH_LOCK(lock_);
    auto it = address_dispatch_map_.find(address_index);
    if (it != address_dispatch_map_.end())
    {
      // dispatch the packet to the subscription feed
      success = it->second.Dispatch(*packet, transmitter);

      if (!success)
      {
        FETCH_LOG_WARN(logging_name_,
                       "Failed to dispatch message to a given subscription (address specific)");
      }
    }
  }

  return success;
}

}  // namespace muddle
}  // namespace fetch
