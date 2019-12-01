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

#include "kademlia/peer_info.hpp"
#include "kademlia/primitives.hpp"

#include <deque>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace muddle {

struct Bucket
{
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using BucketId  = uint64_t;
  using Peer      = std::shared_ptr<PeerInfo>;  // TODO: Make weak

  BucketId                 bucket_id;
  std::unordered_set<Peer> peers;
  Timepoint                last_updated;  // TODO: not used atm

  /*
  // TODO: Move from primitives
  static BucketId GetBucketID(KademliaDistance const &distance)
  {
    BucketId ret{0};

    return ret;
  }
  */
};

}  // namespace muddle
}  // namespace fetch
