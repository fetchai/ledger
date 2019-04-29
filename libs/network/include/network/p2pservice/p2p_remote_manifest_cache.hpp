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

#include "core/future_timepoint.hpp"
#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "network/muddle/packet.hpp"
#include "network/p2pservice/manifest.hpp"

#include <unordered_set>
#include <vector>

namespace fetch {
namespace p2p {

/**
 * This holds a mapping of remote-host to manifest. It also includes a validity time so that this
 * information can be updated periodically.
 */
class ManifestCache
{
public:
  struct CacheEntry
  {
    core::FutureTimepoint timepoint;
    network::Manifest        manifest;
  };

  using Clock      = core::FutureTimepoint::Clock;
  using Timepoint  = Clock::time_point;
  using Manifest   = network::Manifest;
  using Address    = muddle::Packet::Address;
  using Cache      = std::unordered_map<Address, CacheEntry>;
  using Mutex      = mutex::Mutex;
  using Lock       = std::unique_lock<Mutex>;
  using AddressSet = std::unordered_set<Address>;

  // Construction / Destruction
  ManifestCache()                      = default;
  ManifestCache(ManifestCache const &) = delete;
  ManifestCache(ManifestCache &&)      = delete;
  ~ManifestCache()                     = default;

  // Queries
  bool       Get(Address const &address, Manifest &manifest) const;
  AddressSet GetUpdatesNeeded() const;
  AddressSet GetUpdatesNeeded(AddressSet const &addresses) const;

  // Updates
  void ProvideUpdate(Address const &address, network::Manifest const &manifest,
                     std::size_t valid_for);

  // Operators
  ManifestCache &operator=(ManifestCache const &) = delete;
  ManifestCache &operator=(ManifestCache &&) = delete;

private:
  Cache         cache_;
  mutable Mutex mutex_{__LINE__, __FILE__};
};

/**
 * Gets a manifest for specified address
 *
 * @param address The address to be searched for
 * @param manifest The reference to the output manifest
 * @return true if manifest found and returned, otherwise false
 */
inline bool ManifestCache::Get(Address const &address, Manifest &manifest) const
{
  bool success = false;

  {
    FETCH_LOCK(mutex_);

    auto iter = cache_.find(address);
    if (iter != cache_.end())
    {
      manifest = iter->second.manifest;
      success  = true;
    }
  }

  return success;
}

/**
 * Generate a set of address from which manifest updates are required
 *
 * @return The set of addresses
 */
inline ManifestCache::AddressSet ManifestCache::GetUpdatesNeeded() const
{
  Timepoint const now = Clock::now();
  AddressSet      addresses;

  {
    FETCH_LOCK(mutex_);
    for (auto const &entry : cache_)
    {
      if (entry.second.timepoint.IsDue(now))
      {
        addresses.insert(entry.first);
      }
    }
  }

  return addresses;
}

/**
 * Determines which of the given input addresses require updating
 *
 * @param inputs The list of inputs to search
 * @return The vector of addresses to be updated
 */
inline ManifestCache::AddressSet ManifestCache::GetUpdatesNeeded(AddressSet const &addresses) const
{
  Timepoint const now = Clock::now();
  AddressSet      result;

  {
    FETCH_LOCK(mutex_);

    for (auto const &address : addresses)
    {
      auto const it = cache_.find(address);
      if ((it == cache_.end()) || (it->second.timepoint.IsDue(now)))
      {
        result.insert(address);
      }
    }
  }

  return result;
}

/**
 * Updates or adds an entry in the manifest cache
 *
 * @param address The address of the peer
 * @param manifest The manifest for that peer
 * @param valid_for The time in seconds for the cache entry to be valid
 */
inline void ManifestCache::ProvideUpdate(Address const &address, network::Manifest const &manifest,
                                         std::size_t valid_for)
{
  FETCH_LOCK(mutex_);

  CacheEntry &entry = cache_[address];

  entry.manifest = manifest;
  entry.timepoint.SetSeconds(valid_for);
}

}  // namespace p2p
}  // namespace fetch
