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

#include "crypto/prover.hpp"
#include "network/muddle/network_id.hpp"

#include <chrono>
#include <vector>

namespace fetch {
namespace ledger {

struct ShardConfig
{
  using Timeperiod     = std::chrono::milliseconds;
  using CertificatePtr = std::shared_ptr<crypto::Prover>;
  using NetworkId      = muddle::NetworkId;

  /// @name Basic Information
  /// @{
  uint32_t    lane_id;       ///< The lane number
  uint32_t    num_lanes;     ///< The total number of lanes
  std::string storage_path;  ///< The storage path prefix
  /// @}

  /// @name External Network
  /// @{
  CertificatePtr external_identity;    ///< The identity for the external network
  uint16_t       external_port;        ///< The server port for the external network
  NetworkId      external_network_id;  ///< The ID of the external network
  /// @}

  /// @name Internal Network
  /// @{
  CertificatePtr internal_identity;    ///< The identity for the internal network
  uint16_t       internal_port;        ///< The server port for the internal network
  NetworkId      internal_network_id;  ///< The ID of the internal network
  /// @}

  /// @name Tx Sync Configuration
  /// @{
  std::size_t verification_threads{1};  ///< Num threads for tx verification
  Timeperiod  sync_service_timeout{5000};
  Timeperiod  sync_service_promise_timeout{2000};
  Timeperiod  sync_service_fetch_period{5000};
  /// @}
};

using ShardConfigs = std::vector<ShardConfig>;

}  // namespace ledger
}  // namespace fetch
