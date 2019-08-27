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

#include "muddle/address.hpp"
#include "network/uri.hpp"

#include <chrono>
#include <unordered_map>

namespace fetch {
namespace muddle {

class DiscoveryCache
{
public:
  using Uris = std::vector<network::Uri>;

  // Construction / Destruction
  DiscoveryCache()                       = default;
  DiscoveryCache(DiscoveryCache const &) = delete;
  DiscoveryCache(DiscoveryCache &&)      = delete;
  ~DiscoveryCache()                      = default;

  // Operators
  DiscoveryCache &operator=(DiscoveryCache const &) = delete;
  DiscoveryCache &operator=(DiscoveryCache &&) = delete;

private:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;

  struct Entry
  {
    Uris      uris;
    Timepoint timestamp{Clock::now()};

    explicit Entry(Uris u)
      : uris{std::move(u)}
    {}
  };
};

}  // namespace muddle
}  // namespace fetch
