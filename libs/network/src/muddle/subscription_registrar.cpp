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

#include "network/muddle/subscription_registrar.hpp"

namespace fetch {
namespace muddle {
namespace {

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
  index |= static_cast<uint32_t>(service) << 16;
  index |= static_cast<uint32_t>(channel);
  return index;
}

}  // namespace

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
bool SubscriptionRegistrar::Dispatch(PacketPtr packet, Address transmitter)
{
  bool success = false;

  Index const  index = Combine(packet->GetService(), packet->GetProtocol());
  AddressIndex address_index{index, packet->GetTarget()};

  {
    FETCH_LOCK(lock_);
    auto it = dispatch_map_.find(index);
    if (it != dispatch_map_.end())
    {
      // dispatch the packet to the subscription feed
      success =
          it->second.Dispatch(packet->GetSender(), packet->GetService(), packet->GetProtocol(),
                              packet->GetMessageNum(), packet->GetPayload(), transmitter);

      if (!success)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to dispatch message to a given subscription");
      }
    }
  }

  {
    FETCH_LOCK(lock_);
    auto it = address_dispatch_map_.find(address_index);
    if (it != address_dispatch_map_.end())
    {
      // dispatch the packet to the subscription feed
      success =
          it->second.Dispatch(packet->GetSender(), packet->GetService(), packet->GetProtocol(),
                              packet->GetMessageNum(), packet->GetPayload(), transmitter);

      if (!success)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to dispatch message to a given subscription2");
      }
    }
  }

  if (!success)
  {
    Debug("While dispatching");
  }

  return success;
}

void SubscriptionRegistrar::Debug(std::string const &prefix) const
{
  FETCH_LOG_WARN(LOGGING_NAME, prefix,
                 "SubscriptionRegistrar: --------------------------------------");

  FETCH_LOG_WARN(LOGGING_NAME, prefix,
                 "SubscriptionRegistrar:dispatch_map_ = ", dispatch_map_.size(), " entries.");
  FETCH_LOG_WARN(LOGGING_NAME, prefix,
                 "SubscriptionRegistrar:address_dispatch_map_ = ", address_dispatch_map_.size(),
                 " entries.");

  for (const auto &mapping : address_dispatch_map_)
  {
    auto numb = std::get<0>(mapping.first);
    auto addr = std::get<1>(mapping.first);
    FETCH_LOG_WARN(LOGGING_NAME, prefix,
                   "SubscriptionRegistrar:address_dispatch_map_ Addr=", addr.ToBase64(),
                   "  Service=", ((numb >> 16) & 0xFFFF));
  }
  for (const auto &mapping : dispatch_map_)
  {
    auto numb = mapping.first;
    auto serv = (numb >> 16) & 0xFFFF;
    auto chan = numb & 0xFFFF;
    FETCH_LOG_WARN(LOGGING_NAME, prefix, "SubscriptionRegistrar:dispatch_map_ Serv=", serv,
                   "  Chan=", chan);
  }
  FETCH_LOG_WARN(LOGGING_NAME, prefix,
                 ":subscriptionRegistrar: --------------------------------------");
}

}  // namespace muddle
}  // namespace fetch
