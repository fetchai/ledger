#pragma once

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