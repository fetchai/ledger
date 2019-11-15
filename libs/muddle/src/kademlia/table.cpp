#include "muddle/kademlia/table.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/sha1.hpp"

namespace fetch {
namespace muddle {

KademliaTable::KademliaTable(RawAddress const &own_address)
  : own_address_{KademliaAddress::Create(own_address)}
{}

// TODO: This might not be what we want to do
Peers KademliaTable::ProposePermanentConnections() const
{
  FETCH_LOCK(mutex_);

  uint64_t n = 0;
  Peers    ret;
  for (auto &p : buckets_[n].peers)
  {
    ret.push_back(*p);
  }

  ++n;

  while ((n < buckets_.size()) && (ret.size() < kademlia_max_peers_per_bucket_))
  {
    for (auto const &p : buckets_[n].peers)
    {
      ret.push_back(*p);
    }
  }

  return ret;
}

Peers KademliaTable::FindPeer(RawAddress const &address)
{
  FETCH_LOCK(mutex_);

  // Computing the Kademlia distance and the
  // corresponding bucket.
  auto kam_address = KademliaAddress::Create(address);
  auto dist        = GetKademliaDistance(own_address_, kam_address);
  auto bucket_id   = GetBucket(dist);

  assert(bucket_id <= KADEMLIA_MAX_ID_BITS);

  // Getting the list of peers from the bucket
  Peers ret;
  for (auto &p : buckets_[bucket_id].peers)
  {
    ret.push_back(*p);
  }

  // Preparing to search nearby buckets in case there is not
  // enough peers in the current bucket.
  auto left            = bucket_id;
  auto right           = bucket_id;
  bool need_more_peers = ret.size() < kademlia_max_peers_per_bucket_;

  // Checking if we need to go to other buckets to add more
  // peers.
  while (need_more_peers)
  {
    need_more_peers = false;

    // Searhcing the buckets left of the main bucket
    if (left != 0)
    {
      left -= 1;
      for (auto const &p : buckets_[left].peers)
      {
        ret.push_back(*p);
      }

      need_more_peers = ret.size() < kademlia_max_peers_per_bucket_;
    }

    // And the ones right of the main bucket.
    if (right < KADEMLIA_MAX_ID_BITS)
    {
      right += 1;

      for (auto const &p : buckets_[right].peers)
      {
        ret.push_back(*p);
      }

      need_more_peers = ret.size() < kademlia_max_peers_per_bucket_;
    }
  }

  // Worst cast size of the return is at this point
  // 3 * kademlia_max_peers_per_bucket_ - 1. We now
  // trim and sort according to distance
  assert(ret.size() < 3 * kademlia_max_peers_per_bucket_);

  for (auto &peer : ret)
  {
    peer.distance = GetKademliaDistance(peer.kademlia_address, kam_address);
  }

  // Sorting according to distance
  std::sort(ret.begin(), ret.end());

  // Trimming the list
  while (ret.size() > kademlia_max_peers_per_bucket_)
  {
    ret.pop_back();
  }

  return ret;
}

// TODO: add URI
void KademliaTable::ReportLiveliness(RawAddress const &address)
{
  FETCH_LOCK(mutex_);

  auto other     = KademliaAddress::Create(address);
  auto dist      = GetKademliaDistance(own_address_, other);
  auto bucket_id = GetBucket(dist);

  assert(bucket_id <= KADEMLIA_MAX_ID_BITS);

  // Fetching the bucket and finding the peer if it is in the
  // bucket
  auto &bucket  = buckets_[bucket_id];
  auto  peer_it = bucket.peers.begin();
  for (; peer_it != bucket.peers.end(); ++peer_it)
  {
    if (address == (*peer_it)->address)
    {
      break;
    }
  }

  // Finding or creating peer information
  PeerInfoPtr peerinfo;
  if (peer_it != bucket.peers.end())
  {
    // Peer is known and located in a bucket.
    peerinfo = *peer_it;
    bucket.peers.erase(peer_it);
  }
  else
  {
    // Peer is already known but not in any
    // bucket.
    auto it = know_peers_.find(address);
    if (it != know_peers_.end())
    {
      peerinfo = it->second;
    }
    else
    {
      // Discovered a new peer.
      peerinfo                   = std::make_shared<PeerInfo>();
      peerinfo->address          = address;
      peerinfo->kademlia_address = other;
      peerinfo->distance         = dist;
      // TODO: Obtain URI

      // Ensures that peer information persists over time
      // even if the peer disappears from the bucket.
      know_peers_[address] = know_peers_;
    }
  }

  // Updating activity information
  // TODO: This last part is wrong
  peerinfo->message_count += 1;
  // TODO: peerinfo.last_activity
  bucket.peers.push_back(peerinfo);

  // Triming the list
  // TODO: move to after pinging
  while (bucket.peers.size() > kademlia_max_peers_per_bucket_)
  {
    bucket.peers.pop_front();
  }
}

}  // namespace muddle
}  // namespace fetch