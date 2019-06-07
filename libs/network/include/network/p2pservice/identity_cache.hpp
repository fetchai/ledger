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
#include "crypto/fnv.hpp"
#include "network/muddle/muddle.hpp"
#include "network/uri.hpp"

#include <chrono>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace p2p {

class IdentityCache
{
public:
  using Address       = muddle::Muddle::Address;
  using ConnectionMap = muddle::Muddle::ConnectionMap;
  using Uri           = network::Uri;
  using Clock         = std::chrono::steady_clock;
  using Timepoint     = Clock::time_point;
  using Mutex         = mutex::Mutex;
  using AddressSet    = std::unordered_set<Address>;

  struct CacheElement
  {
    Uri       uri;
    Timepoint last_update = Clock::now();
    bool      resolve{false};

    CacheElement(Uri uri)
      : uri(std::move(uri))
    {}
  };

  using Cache = std::unordered_map<Address, CacheElement>;

  // Construction / Destruction
  IdentityCache()                      = default;
  IdentityCache(IdentityCache const &) = delete;
  IdentityCache(IdentityCache &&)      = delete;
  ~IdentityCache()                     = default;

  // Queries and Updates
  void       Update(ConnectionMap const &connections);
  void       Update(Address const &address, Uri const &uri);
  bool       Lookup(Address const &address, Uri &uri) const;
  AddressSet FilterOutUnresolved(AddressSet const &addresses);

  void VisitCache(std::function<void(Cache const &)> cb) const
  {
    if (cb)
    {
      FETCH_LOCK(lock_);
      cb(cache_);
    }
  }

  // Operators
  IdentityCache &operator=(IdentityCache const &) = delete;
  IdentityCache &operator=(IdentityCache &&) = delete;

private:
  void UpdateInternal(Address const &address, Uri const &uri);

  mutable Mutex lock_{__LINE__, __FILE__};
  Cache         cache_;
};

}  // namespace p2p
}  // namespace fetch
