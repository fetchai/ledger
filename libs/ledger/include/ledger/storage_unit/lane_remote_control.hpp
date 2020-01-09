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

#include "core/mutex.hpp"
#include "ledger/shard_config.hpp"
#include "muddle/rpc/client.hpp"
#include "shards/shard_management_interface.hpp"

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ledger {

class LaneRemoteControl : public shards::ShardManagementInterface
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
  /// @name Shard Management
  /// @{
  void UseThesePeers(LaneIndex lane, AddressMap const &addresses) override;
  /// @}

private:
  using RpcClient   = muddle::rpc::Client;
  using Address     = muddle::Address;
  using AddressList = std::vector<Address>;

  Address const &LookupAddress(LaneIndex lane) const;

  AddressList const addresses_;
  RpcClient         rpc_client_;  ///< The RPC client
};

}  // namespace ledger
}  // namespace fetch
