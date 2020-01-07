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

#include "kademlia/primitives.hpp"
#include "moment/clock_interfaces.hpp"
#include "muddle/packet.hpp"

#include <chrono>
#include <cmath>
#include <memory>

namespace fetch {
namespace muddle {

struct AddressPriority
{
  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;

  template <typename T>
  using Duration = typename std::chrono::duration<T>;
  using Address  = Packet::Address;

  enum
  {
    ADDRESS_SIZE         = KademliaAddress::ADDRESS_SIZE,
    KADEMLIA_MAX_ID_BITS = KademliaAddress::KADEMLIA_MAX_ID_BITS
  };

  enum Purpose
  {
    NORMAL,
    PRIORITY_CONNECTION,
  };

  /// Key fields for defining address priority
  /// @{
  Address address{};
  bool    is_connected{false};
  bool    is_incoming{false};  ///< Whether or not the nature of the comms is incoming.
  double  priority{1};
  double  priority_permanent{1};
  double  priority_temporary{1};
  /// @}

  /// Lifetime
  /// @{
  bool      persistent{true};               ///< Whether or not this connnection is permanent
  Timepoint connected_since{Clock::now()};  ///< Time at which connection was established
  Timepoint desired_expiry{Clock::now()};   ///< Time at which this connection is no longer relevant
  /// @}

  /// Distance
  /// @{
  uint64_t bucket{KADEMLIA_MAX_ID_BITS};
  double   connection_value{0};  ///< Overall value obtained from connection.
  /// @}

  void ScheduleDisconnect()
  {
    persistent      = false;
    connected_since = Clock::now();
    desired_expiry  = Clock::now() - std::chrono::seconds(60);

    connection_value = 0;
  }

  void UpdatePriority()
  {
    // TODO(tfr): Get parameters from configuration
    double const params[] = {1.0 / 30., 1. / 20., 0.05, 1.0 / 3600., 10.0};
    Timepoint    now      = Clock::now();

    // Priority goes down exponentially with the increasing bucket number.
    auto bucket_d = static_cast<double>(bucket);
    if (bucket_d > static_cast<double>(KADEMLIA_MAX_ID_BITS))
    {
      bucket_d = static_cast<double>(KADEMLIA_MAX_ID_BITS);
    }

    assert((0 <= bucket_d) && (bucket_d <= static_cast<double>(KADEMLIA_MAX_ID_BITS)));

    double bucket_coef_perm =
        1.0 / (1 + exp(-params[2] * (static_cast<double>(KADEMLIA_MAX_ID_BITS) / 2. - bucket_d)));

    // We value long time connections
    double connection_time =
        std::chrono::duration_cast<Duration<double>>(now - connected_since).count();
    double connect_coef = 1. / (1. + exp(-params[3] * connection_time));

    // Converges to 1 when connection value is large and positive
    // Exactly 0.5 when the connection has no value
    // Goes rapidly to zero when connection value is negative
    double behaviour_coef = 1. / (1 + exp(-params[4] * connection_value));

    // Setting priority
    priority = behaviour_coef * bucket_coef_perm * connect_coef;
  }

  bool operator<(AddressPriority const &other) const
  {
    return priority < other.priority;
  }
};  // namespace muddle
}  // namespace muddle
}  // namespace fetch
