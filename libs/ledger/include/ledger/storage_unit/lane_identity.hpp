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

#include <utility>

#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/service_client.hpp"

namespace fetch {
namespace ledger {

class LaneIdentity
{
public:
  using ConnectivityDetailsType = LaneConnectivityDetails;
  using NetworkManagerType      = fetch::network::NetworkManager;
  using PingType                = uint32_t;
  using LaneType                = uint32_t;

  static constexpr char const *LOGGING_NAME = "LaneIdentity";

  LaneIdentity(NetworkManagerType const &nm, crypto::Identity identity)
    : identity_(std::move(identity))
    , manager_(nm)
  {
    lane_        = uint32_t(-1);
    total_lanes_ = 0;
  }

  /// External controls
  /// @{

  crypto::Identity Identity()
  {
    FETCH_LOCK(identity_mutex_);
    return identity_;
  }

  LaneType GetLaneNumber()
  {
    return lane_;
  }

  LaneType GetTotalLanes()
  {
    return total_lanes_;
  }

  /// @}

  /// Internal controls
  /// @{
  void SetLaneNumber(LaneType const &lane)
  {
    lane_ = lane;
  }

  void SetTotalLanes(LaneType const &t)
  {
    total_lanes_ = t;
  }
  /// @}

private:
  Mutex            identity_mutex_;
  crypto::Identity identity_;

  NetworkManagerType manager_;

  std::atomic<LaneType> lane_;
  std::atomic<LaneType> total_lanes_;
};

}  // namespace ledger
}  // namespace fetch
