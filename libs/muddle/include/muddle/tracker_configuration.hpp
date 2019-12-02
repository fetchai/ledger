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

#include "network/service/promise.hpp"

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
    return conf;
  }

  static TrackerConfiguration DefaultConfiguration()
  {
    TrackerConfiguration conf;

    conf.allow_desired_connections = true;
    conf.register_connections      = false;
    conf.pull_peers                = false;
    conf.connect_to_nearest        = false;
    conf.disconnect_duplicates     = false;
    conf.trim_peer_list            = false;
    return conf;
  }

  /// Operations
  /// @{
  bool allow_desired_connections{true};
  bool register_connections{true};
  bool pull_peers{true};
  bool connect_to_nearest{true};
  bool disconnect_duplicates{true};
  bool trim_peer_list{true};
  bool long_range_connectivity{true};
  /// @}

  uint64_t max_kademlia_connections{3};
  uint64_t max_longrange_connections{1};
  uint64_t max_desired_connections{2};
  uint64_t max_discovery_connections{2};

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
