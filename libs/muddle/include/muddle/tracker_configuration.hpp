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

#include "network/service/promise.hpp"

#include <sstream>

namespace fetch {
namespace muddle {

struct TrackerConfiguration
{
  using Clock     = service::details::PromiseImplementation::Clock;
  using Duration  = Clock::duration;
  using Timepoint = Clock::time_point;

  enum TrackingMode
  {
    NO_TRACKING,
    ASYNC_FORK_AND_COLLECT
  };

  /*
   * @brief Creates a configuration where all features are turned off.
   */
  static TrackerConfiguration AllOff()
  {
    TrackerConfiguration conf;

    conf.allow_desired_connections = false;
    conf.register_connections      = false;
    conf.pull_peers                = false;
    conf.connect_to_nearest        = false;
    conf.disconnect_duplicates     = false;
    conf.trim_peer_list            = false;
    conf.disconnect_from_self      = false;
    conf.allow_connection_expiry   = false;
    return conf;
  }

  static TrackerConfiguration AllOn()
  {
    TrackerConfiguration conf;

    conf.allow_desired_connections = true;
    conf.register_connections      = true;
    conf.pull_peers                = true;
    conf.connect_to_nearest        = true;
    conf.disconnect_duplicates     = true;
    conf.trim_peer_list            = true;
    conf.long_range_connectivity   = true;
    conf.disconnect_from_self      = true;
    conf.allow_connection_expiry   = true;
    return conf;
  }

  static TrackerConfiguration DefaultConfiguration()
  {
    TrackerConfiguration conf;

    conf.allow_desired_connections = true;
    conf.register_connections      = true;
    conf.pull_peers                = false;
    conf.connect_to_nearest        = false;
    conf.disconnect_duplicates     = true;
    conf.trim_peer_list            = false;
    conf.long_range_connectivity   = false;
    conf.disconnect_from_self      = true;
    conf.allow_connection_expiry   = true;
    return conf;
  }

  std::string ToString()
  {
    std::stringstream ss{""};
    ss << "allow_desired_connections: " << allow_desired_connections << std::endl;
    ss << "register_connections: " << register_connections << std::endl;
    ss << "pull_peers: " << pull_peers << std::endl;
    ss << "connect_to_nearest: " << connect_to_nearest << std::endl;
    ss << "disconnect_duplicates: " << disconnect_duplicates << std::endl;
    ss << "trim_peer_list: " << trim_peer_list << std::endl;
    ss << "long_range_connectivity: " << long_range_connectivity << std::endl;
    ss << "disconnect_from_self: " << disconnect_from_self << std::endl;
    ss << "allow_connection_expiry: " << allow_connection_expiry << std::endl;

    return ss.str();
  }

  /// Operations
  /// @{
  bool allow_desired_connections{true};
  bool register_connections{true};
  bool pull_peers{false};
  bool connect_to_nearest{false};
  bool disconnect_duplicates{true};
  bool trim_peer_list{false};
  bool long_range_connectivity{false};
  bool disconnect_from_self{true};
  bool allow_connection_expiry{true};
  /// @}

  uint64_t max_kademlia_connections{5};
  uint64_t max_longrange_connections{5};
  uint64_t max_desired_connections{255};
  int64_t  max_discovery_tasks{3};

  /// Priority paramters
  /// @{
  double expiry_decay{1.0 / 30.};
  double bucket_decay{1.0 / 20.};
  double connectivity_decay{1.0 / 3600};
  double behaviour_decay{10.0};
  /// @}

  /// Kademlia
  /// @{
  uint64_t kademlia_bucket_size{20};
  uint64_t kademlia_bucket_count{160};
  /// @}

  /// Connectivity
  /// @{
  //  uint64_t max_shortlived_outgoing{2};
  //  uint64_t max_longlived_outgoing{2};
  /// @}

  /// RPC configuration
  /// @{
  Duration promise_timeout{std::chrono::duration_cast<Duration>(std::chrono::seconds{1})};
  /// @}

  /// Tracking
  /// @{
  TrackingMode tracking_mode;
  uint64_t     async_calls{5};
  Duration     periodicity;
  Duration     default_connection_expiry{
      std::chrono::duration_cast<Duration>(std::chrono::seconds{20})};
  /// @}
};

}  // namespace muddle
}  // namespace fetch
