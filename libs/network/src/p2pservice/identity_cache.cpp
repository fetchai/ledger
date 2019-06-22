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

#include "network/p2pservice/identity_cache.hpp"

#include <algorithm>
#include <iterator>

namespace fetch {
namespace p2p {

void IdentityCache::Update(ConnectionMap const &connections)
{
  FETCH_LOCK(lock_);

  for (auto const &element : connections)
  {
    auto const &address = element.first;
    auto const &uri     = element.second;

    UpdateInternal(address, uri);
  }
}

void IdentityCache::Update(Address const &address, Uri const &uri)
{
  FETCH_LOCK(lock_);
  UpdateInternal(address, uri);
}

bool IdentityCache::Lookup(Address const &address, Uri &uri) const
{
  bool success = false;

  FETCH_LOCK(lock_);
  auto it = cache_.find(address);
  if (it != cache_.end())
  {
    uri     = it->second.uri;
    success = true;
  }

  return success;
}

IdentityCache::AddressSet IdentityCache::FilterOutUnresolved(AddressSet const &addresses)
{
  AddressSet resolvedAddresses;

  {
    FETCH_LOCK(lock_);

    std::copy_if(addresses.begin(), addresses.end(),
                 std::inserter(resolvedAddresses, resolvedAddresses.begin()),
                 [this](Address const &address) {
                   bool resolved = false;

                   auto it = cache_.find(address);
                   if ((it != cache_.end()) && (it->second.uri.scheme() != Uri::Scheme::Muddle))
                   {
                     resolved = true;
                   }

                   return resolved;
                 });
  }

  return resolvedAddresses;
}

void IdentityCache::UpdateInternal(Address const &address, Uri const &uri)
{
  auto cache_it = cache_.find(address);
  if (cache_it != cache_.end())
  {
    auto &cache_entry = cache_it->second;

    // if the cache entry exists them only update it if the
    if (uri.scheme() != Uri::Scheme::Muddle)
    {
      cache_entry.uri         = uri;
      cache_entry.last_update = Clock::now();
      cache_entry.resolve     = false;  // This entry is considered resolved
    }
  }
  else
  {
    cache_.emplace(address, uri);
  }
}

}  // namespace p2p
}  // namespace fetch
