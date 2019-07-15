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

#include "core/mutex.hpp"
#include "ledger/shard_config.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/p2pservice/p2p_lane_management.hpp"

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ledger {

class LaneRemoteControl : public p2p::LaneManagement
{
public:
  static constexpr char const *LOGGING_NAME = "LaneRemoteControl";

  using MuddleEndpoint = muddle::MuddleEndpoint;

  // Construction / Destruction
  LaneRemoteControl(MuddleEndpoint &endpoint, ShardConfigs const &shards, uint32_t log2_num_lanes);
  LaneRemoteControl(LaneRemoteControl const &other) = delete;
  LaneRemoteControl(LaneRemoteControl &&other)      = delete;
  ~LaneRemoteControl() override                     = default;

  // Operators
  LaneRemoteControl &operator=(LaneRemoteControl const &other) = delete;
  LaneRemoteControl &operator=(LaneRemoteControl &&other) = delete;

protected:
  /// @name Lane Management
  /// @{
  void     UseThesePeers(LaneIndex lane, std::unordered_set<Uri> const &uris) override;
  void     Shutdown(LaneIndex lane) override;
  uint32_t GetLaneNumber(LaneIndex lane) override;
  int      IncomingPeers(LaneIndex lane) override;
  int      OutgoingPeers(LaneIndex lane) override;
  bool     IsAlive(LaneIndex lane) override;
  /// @}

private:
  using RpcClient   = muddle::rpc::Client;
  using Address     = muddle::MuddleEndpoint::Address;
  using AddressList = std::vector<Address>;

  Address const &LookupAddress(LaneIndex lane) const;

  AddressList const addresses_;
  RpcClient         rpc_client_;  ///< The RPC client
};

}  // namespace ledger
}  // namespace fetch
