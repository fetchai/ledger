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

#include "core/containers/set_difference.hpp"
#include "core/service_ids.hpp"
#include "crypto/identity.hpp"
#include "logging/logging.hpp"
#include "muddle/muddle_interface.hpp"
#include "shards/shard_management_interface.hpp"
#include "shards/shard_management_service.hpp"

using namespace std::chrono_literals;

namespace fetch {
namespace shards {
namespace {

using AddressHints    = muddle::MuddleInterface::AddressHints;
using ShardAddressCfg = std::vector<AddressHints>;

auto const REFRESH_THRESHOLD = 5min;
auto const STALE_THRESHOLD   = 15min;

constexpr char const *LOGGING_NAME = "ShardMgmt";

}  // namespace

ShardManagementService::ShardManagementService(Manifest manifest, ShardManagementInterface &shards,
                                               MuddleInterface &muddle, uint32_t log2_num_lanes)
  : core::PeriodicRunnable(1s)
  , shards_{shards}
  , muddle_{muddle}
  , manifest_{std::move(manifest)}
  , log2_num_shards_{log2_num_lanes}
  , num_shards_{1u << log2_num_shards_}
  , rpc_server_{muddle_.GetEndpoint(), SERVICE_SHARD_MGMT, CHANNEL_RPC}
  , mgmt_proto_{*this}
  , rpc_client_{"MgmtRpc", muddle_.GetEndpoint(), SERVICE_SHARD_MGMT, CHANNEL_RPC}
{
  // register the server protocol
  rpc_server_.Add(RPC_SHARD_MGMT, &mgmt_proto_);
}

bool ShardManagementService::QueryManifest(Address const &address, Manifest &manifest)
{
  bool success{false};

  FETCH_LOCK(lock_);

  // attempt to look up the manifest from the cache
  auto it = manifest_cache_.find(address);

  if (it == manifest_cache_.end())
  {
    // add the address to the pending set
    if (pending_requests_.find(address) != pending_requests_.end())
    {
      unavailable_requests_.emplace(address);
    }
  }
  else
  {
    manifest = it->second.manifest;
    success  = true;
  }

  return success;
}

Manifest ShardManagementService::RequestManifest()
{
  return manifest_;
}

void ShardManagementService::Periodically()
{
  FETCH_LOG_TRACE(LOGGING_NAME, "### Shard Management Periodical ###");
  FETCH_LOCK(lock_);

  // resolve previous requests
  ResolveUpdates();

  // evaluate the nodes required to update
  Addresses const addresses            = muddle_.GetOutgoingConnectedPeers();
  Addresses const unresolved_addresses = (addresses - manifest_cache_) - pending_requests_;

  // request updates on the unresolved entries
  RequestUpdates(unresolved_addresses);

  // update the shard configuration
  UpdateShards(addresses);

  // clean up
  RefreshCache();

  FETCH_LOG_TRACE(LOGGING_NAME, "### Shard Management Periodical (Complete) ###");
}

void ShardManagementService::ResolveUpdates()
{
  auto const now = Clock::now();

  // loop through all the outstanding requests
  auto    it = pending_requests_.begin();
  Address current_peer;

  try
  {
    while (it != pending_requests_.end())
    {
      current_peer = it->first;
      if (it->second->IsSuccessful())
      {
        FETCH_LOG_TRACE(LOGGING_NAME, "Resolved manifest from: ", it->first.ToBase64());

        // look up the cache entry
        auto &entry = manifest_cache_[it->first];

        // update the cache entry
        entry.manifest     = it->second->As<Manifest>();
        entry.last_updated = now;

        // remove the promise from the queue
        it = pending_requests_.erase(it);
      }
      else if (it->second->IsFailed())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to get manifest from ", it->first.ToBase64());
        it = pending_requests_.erase(it);
      }
      else
      {
        // continue waiting for the promise to resolve
        ++it;
      }
    }
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to request manifest from peer: ", current_peer,
                   " exception thrown: ", e.what());
  }
}

void ShardManagementService::RequestUpdates(Addresses addresses)
{
  // update the requested addresses with any unavailable queries
  for (auto const &address : unavailable_requests_)
  {
    addresses.emplace(address);
  }
  unavailable_requests_.clear();

  // strip and pending
  addresses = addresses - pending_requests_;

  // request the manifest from the address
  for (auto const &address : addresses)
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Request manifest: ", address.ToBase64());

    pending_requests_.emplace(
        address, rpc_client_.CallSpecificAddress(address, RPC_SHARD_MGMT,
                                                 ShardManagementProtocol::REQUEST_MANIFEST));
  }
}

void ShardManagementService::UpdateShards(Addresses const &addresses)
{
  // early exit
  if (addresses.empty())
  {
    return;
  }

  ShardAddressCfg shard_address_cfg(static_cast<std::size_t>(num_shards_));

  // based on the resolved addresses build up the array of peer connection maps which can be
  // passed on to the corresponding lanes
  for (auto const &address : addresses)
  {
    auto it = manifest_cache_.find(address);
    if (it != manifest_cache_.end())
    {
      auto const &manifest = it->second.manifest;

      for (uint32_t shard = 0; shard < num_shards_; ++shard)
      {
        // look up the give output shard configuration
        auto &shard_cfg = shard_address_cfg[static_cast<std::size_t>(shard)];

        // attempt to locate the corresponding entry in the manifest
        auto shard_it =
            manifest.FindService(ServiceIdentifier{ServiceIdentifier::Type::LANE, shard});

        if ((shard_it != manifest.end()) && !shard_it->second.address().empty())
        {
          // add to the configuration
          shard_cfg.emplace(shard_it->second.address(), shard_it->second.uri());
        }
      }
    }
  }

  // update all the shards
  for (uint32_t i = 0, end = static_cast<uint32_t>(shard_address_cfg.size()); i < end; ++i)
  {
    shards_.UseThesePeers(i, shard_address_cfg[i]);
  }
}

void ShardManagementService::RefreshCache()
{
  Addresses updates;

  // cache the time now so consistent
  auto const now = Clock::now();

  auto it = manifest_cache_.begin();
  while (it != manifest_cache_.end())
  {
    // compute the age of this entry
    auto const age = now - it->second.last_updated;

    if (age >= STALE_THRESHOLD)
    {
      // remove old entries
      it = manifest_cache_.erase(it);
    }
    else
    {
      // if the entry needs to be refreshed then add it to the set
      if (age >= REFRESH_THRESHOLD)
      {
        updates.emplace(it->first);
      }

      // advance the iteration
      ++it;
    }
  }

  // request the cache updates
  RequestUpdates(updates);
}

}  // namespace shards
}  // namespace fetch
