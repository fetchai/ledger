#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
  using connectivity_details_type = LaneConnectivityDetails;
  using client_register_type      = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type      = fetch::network::NetworkManager;
  using mutex_type                = fetch::mutex::Mutex;
  using connection_handle_type    = client_register_type::connection_handle_type;
  using ping_type                 = uint32_t;
  using lane_type                 = uint32_t;

  static constexpr char const *LOGGING_NAME = "LaneIdentity";

  enum
  {
    PING_MAGIC = 1337
  };

  LaneIdentity(client_register_type reg, network_manager_type const &nm, crypto::Identity identity)
    : identity_(std::move(identity))
    , register_(std::move(reg))
    , manager_(nm)
  {
    lane_        = uint32_t(-1);
    total_lanes_ = 0;
  }

  /// External controls
  /// @{
  ping_type Ping()
  {
    return PING_MAGIC;
  }

  crypto::Identity Hello(connection_handle_type const &client, crypto::Identity const &iden)
  {
    auto details = register_.GetDetails(client);

    if (!details)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"Failed to find client in client register! ", __FILE__, " ", __LINE__);
      assert(details);
    }
    else
    // TODO(issue 24): Verify identity if already exists
    {
      std::lock_guard<mutex::Mutex> lock(*details);
      details->identity = iden;
      details->is_peer  = true;
    }

    std::lock_guard<mutex::Mutex> lock(identity_mutex_);
    return identity_;
  }

  void AuthenticateController(connection_handle_type const &client)
  {
    auto details = register_.GetDetails(client);
    {
      std::lock_guard<mutex_type> lock(*details);
      details->is_controller = true;
    }
  }

  crypto::Identity Identity()
  {
    std::lock_guard<mutex::Mutex> lock(identity_mutex_);
    return identity_;
  }

  lane_type GetLaneNumber()
  {
    return lane_;
  }

  lane_type GetTotalLanes()
  {
    return total_lanes_;
  }

  /// @}

  /// Internal controls
  /// @{
  void SetLaneNumber(lane_type const &lane)
  {
    lane_ = lane;
  }

  void SetTotalLanes(lane_type const &t)
  {
    total_lanes_ = t;
  }
  /// @}
  using callable_sign_message_type =
      std::function<byte_array::ConstByteArray(byte_array::ConstByteArray const &)>;

  void OnSignMessage(callable_sign_message_type const &fnc)
  {
    on_sign_message_ = fnc;
  }

private:
  mutex::Mutex               identity_mutex_{__LINE__, __FILE__};
  crypto::Identity           identity_;
  callable_sign_message_type on_sign_message_;

  client_register_type register_;
  network_manager_type manager_;

  std::atomic<lane_type> lane_;
  std::atomic<lane_type> total_lanes_;
};

}  // namespace ledger
}  // namespace fetch
