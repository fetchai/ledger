#pragma once
#include "kademlia/primitives.hpp"
#include "muddle/packet.hpp"

#include <chrono>
#include <cmath>
#include <memory>

namespace fetch {
namespace muddle {

struct AddressPriority
{
  using Clock     = std::chrono::system_clock;
  using TimePoint = Clock::time_point;
  template <typename T>
  using Duration = typename std::chrono::duration<T>;
  using Address  = Packet::Address;

  // TODO: Move all parameters up here.

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
  // TODO: rename persistent to long-lived
  bool      persistent{false};              ///< Whether or not this connnection is permanent
  TimePoint connected_since{Clock::now()};  ///< Time at which connection was established
  TimePoint desired_expiry{Clock::now()};   ///< Time at which this connection is no longer relevant
  /// @}

  /// Distance
  /// @{
  uint64_t bucket{static_cast<uint64_t>(160)};  // TODO: System parameter
  double   connection_value{0};                 ///< Overall value obtained from connection.
  /// @}

  /// Purpose
  /// @{
  Purpose purpose{Purpose::NORMAL};
  /// @}

  bool PreferablyPersistent() const
  {
    if (priority_permanent < 0.02)
    {
      return false;
    }
    return priority_temporary < priority_permanent;
  }

  bool PreferablyTemporary() const
  {
    if (priority_permanent < 0.02)
    {
      return true;
    }
    return priority_temporary > priority_permanent;
  }

  void MakeTemporary()
  {
    persistent = false;
  }

  void MakePersistent()
  {
    persistent = true;
  }

  void ScheduleDisconnect()
  {
    persistent      = false;
    connected_since = Clock::now();
    desired_expiry  = Clock::now() - std::chrono::seconds(60);

    connection_value = 0;
    purpose          = NORMAL;
  }

  void UpdatePriority(bool up_down_grade = false)
  {
    // Consensus connections are considered
    // special high priority connections
    if (purpose == Purpose::PRIORITY_CONNECTION)
    {
      priority = 1.0;
      return;
    }

    double const params[] = {1.0 / 30., 1. / 20., 0.05, 1.0 / 3600., 10.0};
    TimePoint    now      = Clock::now();

    // Compute prioity based on desired expiry
    double time_until_expiry_tmp =
        std::chrono::duration_cast<Duration<double>>(desired_expiry - now).count();
    double time_until_expiry_perm =
        3600. * 24.;  // We use a day as default for persistent connections

    // Becomes 1 when connection is far from its expiry point
    // 0.5 when on its expiry point
    // Goes to zero as expiry is passed.
    double expiry_coef_tmp  = 1. / (1. + exp(-params[0] * time_until_expiry_tmp));
    double expiry_coef_perm = 1. / (1. + exp(-params[0] * time_until_expiry_perm));

    // Priority goes down exponentially with the increasing bucket number.
    double bucket_d = static_cast<double>(bucket);

    // If it is a non-persistent connection, the bucket is less
    // relevant and, in fact, far-away short lived connections has
    // higher priority than long lived far away connections
    double bucket_tmp_param = 1. - expiry_coef_tmp;
    if (params[2] < bucket_tmp_param)
    {
      bucket_tmp_param = params[2];
    }

    assert((0 <= bucket_d) && (bucket_d <= 160.));

    double bucket_coef_tmp  = 1.0 / (1 + exp(-params[2] * (80 - bucket_d)));
    double bucket_coef_perm = 1.0 / (1 + exp(-bucket_tmp_param * (80 - bucket_d)));

    // We value long time connections
    double connection_time =
        std::chrono::duration_cast<Duration<double>>(now - connected_since).count();
    double connect_coef = 1. / (1. + exp(-params[3] * connection_time));

    // Converges to 1 when connection value is large and positive
    // Exactly 0.5 when the connection has no value
    // Goes rapidly to zero when connection value is negative
    double behaviour_coef = 1. / (1 + exp(-params[4] * connection_value));

    // Setting priority
    priority_permanent = expiry_coef_perm * behaviour_coef * bucket_coef_perm * connect_coef;
    priority_temporary = expiry_coef_tmp * behaviour_coef * bucket_coef_tmp * connect_coef;
    // Upgrading or downgrading if requested. We can only upgrade
    if (up_down_grade)
    {
      if (is_incoming)
      {
        // We do not control incoming connections and they
        // are treated as non-persistent.
        persistent = false;
      }
      else if (PreferablyPersistent())
      {
        // If upgrading we do so
        persistent = true;
      }
      else
      {
        // And otherwise we downgrade
        persistent = false;
      }
    }

    // Seting priority depending on whether persistent
    // or not.
    if (persistent)
    {
      priority = priority_permanent;
    }
    else
    {
      priority = priority_temporary;
    }
  }

  bool operator<(AddressPriority const &other) const
  {
    return priority < other.priority;
  }
};  // namespace muddle
}  // namespace muddle
}  // namespace fetch