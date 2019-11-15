#pragma once

#include "muddle/kademlia/peer_info.hpp"
#include "muddle/kademlia/primitives.hpp"

#include <deque>
#include <vector>

namespace fetch {
namespace muddle {

struct Bucket
{
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using BucketId  = uint64_t;
  using Peer      = std::shared_ptr<PeerInfo>;

  BucketId         bucket_id;
  std::deque<Peer> peers;
  Timepoint        last_updated;

  static BucketId GetBucketID(KademliaDistance const &distance)
  {
    BucketId ret{0};

    return ret;
  }
};

}  // namespace muddle
}  // namespace fetch