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
#include "core/periodic_runnable.hpp"
#include "muddle/address.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "shards/manifest.hpp"
#include "shards/manifest_cache_interface.hpp"
#include "shards/shard_management_protocol.hpp"

#include <chrono>

namespace fetch {
namespace muddle {

class MuddleInterface;

}  // namespace muddle
namespace shards {

class ShardManagementInterface;

class ShardManagementService : public core::PeriodicRunnable, public ManifestCacheInterface
{
public:
  using MuddleInterface = muddle::MuddleInterface;
  using Addresses       = MuddleInterface::Addresses;

  // Construction / Destruction
  ShardManagementService(Manifest manifest, ShardManagementInterface &shards,
                         MuddleInterface &muddle, uint32_t log2_num_lanes);
  ShardManagementService(ShardManagementService const &) = delete;
  ShardManagementService(ShardManagementService &&)      = delete;
  ~ShardManagementService() override                     = default;

  // Manifest Queries
  bool QueryManifest(Address const &address, Manifest &manifest) override;

  // External Operations
  Manifest RequestManifest();

  // Operators
  ShardManagementService &operator=(ShardManagementService const &) = delete;
  ShardManagementService &operator=(ShardManagementService &&) = delete;

private:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using RpcServer = muddle::rpc::Server;
  using RpcClient = muddle::rpc::Client;

  struct Entry
  {
    Manifest  manifest{};
    Timepoint last_updated{Clock::now()};
  };

  using ManifestCache   = std::unordered_map<Address, Entry>;
  using PendingPromises = std::unordered_map<Address, service::Promise>;

  /// @name Periodic Runnable Interface
  /// @{
  void Periodically() override;
  void ResolveUpdates();
  void RequestUpdates(Addresses addresses);
  void UpdateShards(Addresses const &addresses);
  void RefreshCache();
  /// @}

  ShardManagementInterface &shards_;
  MuddleInterface &         muddle_;
  Manifest                  manifest_;
  uint32_t                  log2_num_shards_;
  uint32_t                  num_shards_;
  RpcServer                 rpc_server_;
  ShardManagementProtocol   mgmt_proto_;
  RpcClient                 rpc_client_;

  /// @name Manifest Cache
  /// @{
  mutable Mutex   lock_;
  ManifestCache   manifest_cache_;
  Addresses       unavailable_requests_;
  PendingPromises pending_requests_;
  /// @}
};

}  // namespace shards
}  // namespace fetch
