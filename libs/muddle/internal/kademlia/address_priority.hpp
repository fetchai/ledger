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

#include "kademlia/primitives.hpp"
#include "muddle/packet.hpp"

#include <chrono>
#include <cmath>
#include <memory>

namespace fetch {
namespace muddle {

struct AddressPriority
{
  using Clock     = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
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
  TimePoint connected_since{Clock::now()};  ///< Time at which connection was established
  TimePoint desired_expiry{Clock::now()};   ///< Time at which this connection is no longer relevant
  /// @}

  /// Distance
  /// @{
  uint64_t bucket{ADDRESS_SIZE};
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
    TimePoint    now      = Clock::now();

    // Compute prioity based on desired expiry
    double time_until_expiry_perm =
        3600. * 24.;  // We use a day as default for persistent connections

    // Becomes 1 when connection is far from its expiry point
    // 0.5 when on its expiry point
    // Goes to zero as expiry is passed.
    double expiry_coef_perm = 1. / (1. + exp(-params[0] * time_until_expiry_perm));

    // Priority goes down exponentially with the increasing bucket number.
    auto bucket_d = static_cast<double>(bucket);
    if (bucket_d > KADEMLIA_MAX_ID_BITS)
    {
      bucket_d = KADEMLIA_MAX_ID_BITS;
    }

    // If it is a non-persistent connection, the bucket is less
    // relevant and, in fact, far-away short lived connections has
    // higher priority than long lived far away connections
    double bucket_tmp_param = 1. - expiry_coef_perm;
    if (params[2] < bucket_tmp_param)
    {
      bucket_tmp_param = params[2];
    }

    assert((0 <= bucket_d) && (bucket_d <= static_cast<double>(KADEMLIA_MAX_ID_BITS)));

    double bucket_coef_perm =
        1.0 /
        (1 + exp(-bucket_tmp_param * (static_cast<double>(KADEMLIA_MAX_ID_BITS) / 2. - bucket_d)));

    // We value long time connections
    double connection_time =
        std::chrono::duration_cast<Duration<double>>(now - connected_since).count();
    double connect_coef = 1. / (1. + exp(-params[3] * connection_time));

    // Converges to 1 when connection value is large and positive
    // Exactly 0.5 when the connection has no value
    // Goes rapidly to zero when connection value is negative
    double behaviour_coef = 1. / (1 + exp(-params[4] * connection_value));

    // Setting priority
    priority = expiry_coef_perm * behaviour_coef * bucket_coef_perm * connect_coef;
  }

  bool operator<(AddressPriority const &other) const
  {
    return priority < other.priority;
  }
};  // namespace muddle
}  // namespace muddle
}  // namespace fetch
