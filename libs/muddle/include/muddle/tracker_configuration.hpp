#pragma once
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
  uint64_t max_shortlived_outgoing{2};
  uint64_t max_longlived_outgoing{2};
  /// @}

  /// RPC configuration
  /// @{
  Duration promise_timeout{std::chrono::duration_cast<Duration>(std::chrono::seconds{1})};
  /// @}

  /// Tracking
  /// @{
  TrackingMode tracking_mode;
  uint64_t     async_calls{1};
  Duration     periodicity;
  Duration     default_connection_expiry{
      std::chrono::duration_cast<Duration>(std::chrono::seconds{20})};
  /// @}
};

}  // namespace muddle
}  // namespace fetch