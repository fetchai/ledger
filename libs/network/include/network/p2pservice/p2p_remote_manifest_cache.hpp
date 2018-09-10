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

#include "network/generics/future_timepoint.hpp"
#include "core/mutex.hpp"

namespace fetch {
namespace p2p {

/*******
 * This holds a mapping of remote-host to manifest/next-polling-time
 *
 */

class P2PRemoteManifestCache
{
  using Clock = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Identity = crypto::Identity;
  using Stored   = std::pair<network::FutureTimepoint, network::Manifest>;
  using Store    = std::map<Identity, Stored>;
  using Mutex    = mutex::Mutex;
  using Lock     = std::lock_guard<Mutex>;

public:
  P2PRemoteManifestCache()
  {
  }

  std::list<Identity> GetUpdatesNeeded()
  {
    auto now = Clock::now();
    std::list<Identity> res;
    Lock lock(mutex_);
    for( auto& data : data_ )
    {
      if (data.second.first.IsDue(now))
      {
        res . push_back(data.first);
      }
    }
    return res;
  }

  std::list<Identity> GetUpdatesNeeded(const std::list<Identity> &inputs)
  {
    auto now = Clock::now();
    std::list<Identity> res;
    Lock lock(mutex_);

    for( auto& input : inputs )
    {
      auto ref = data_.find(input);
      if (ref == data_.end() || ref -> second.first.IsDue(now))
      {
        res.push_back(input);
      }
    }
    return res;
  }

  void ProvideUpdate(Identity &id, network::Manifest &manif, size_t valid_for_seconds)
  {
    data_[id].second = manif;
    data_[id].first.SetSeconds(valid_for_seconds);
  }

private:
  Store data_;
  Mutex mutex_{__LINE__, __FILE__};
};


}
}
