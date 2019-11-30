#include "kademlia/table.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/sha1.hpp"

namespace fetch {
namespace muddle {

KademliaTable::KademliaTable(Address const &own_address)
  : own_address_{KademliaAddress::Create(own_address)}
{}

// TODO: This might not be what we want to do
KademliaTable::Peers KademliaTable::ProposePermanentConnections() const
{
  FETCH_LOCK(mutex_);

  uint64_t n = 0;
  Peers    ret;
  for (auto &p : by_logarithm_[n].peers)
  {
    ret.push_back(*p);
  }

  ++n;

  while ((n < by_logarithm_.size()) && (ret.size() < kademlia_max_peers_per_bucket_))
  {
    for (auto const &p : by_logarithm_[n].peers)
    {
      ret.push_back(*p);
    }
  }

  return ret;
}

KademliaTable::Peers KademliaTable::FindPeerInternal(KademliaAddress const &kam_address,
                                                     uint64_t log_id, bool scan_left,
                                                     bool scan_right)
{
  // Checking that we are within bounds
  if (log_id > KADEMLIA_MAX_ID_BITS)
  {
    return {};
  }

  // Creating initial list
  std::unordered_map<Address, PeerInfo> results;
  for (auto &p : by_logarithm_[log_id].peers)
  {
    results.insert({p->address, *p});
  }

  // Preparing to search nearby buckets in case there is not
  // enough peers in the current bucket.
  auto left            = log_id;
  auto right           = log_id;
  bool need_more_peers = results.size() < kademlia_max_peers_per_bucket_;

  // Checking if we need to go to other buckets to add more
  // peers.
  while (need_more_peers)
  {
    need_more_peers = false;

    // Searhcing the buckets left of the main bucket
    if (scan_left && (left != 0))
    {
      left -= 1;
      for (auto const &p : by_logarithm_[left].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket_;
    }

    // And the ones right of the main bucket.
    if (scan_right && (right < KADEMLIA_MAX_ID_BITS))
    {
      right += 1;

      for (auto const &p : by_logarithm_[right].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket_;
    }
  }

  // Worst cast size of the return is at this point
  // 3 * kademlia_max_peers_per_bucket_ - 1. We now
  // trim and sort according to distance
  assert(results.size() < 3 * kademlia_max_peers_per_bucket_);
  Peers ret;
  for (auto &peer : results)
  {
    peer.second.distance = GetKademliaDistance(peer.second.kademlia_address, kam_address);
    ret.push_back(peer.second);
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

KademliaTable::Peers KademliaTable::FindPeer(Address const &address)
{
  FETCH_LOCK(mutex_);

  // Computing the Kademlia distance and the
  // corresponding bucket.
  auto kam_address = KademliaAddress::Create(address);
  auto dist        = GetKademliaDistance(own_address_, kam_address);
  auto log_id      = GetBucketByLogarithm(dist);

  return FindPeerInternal(kam_address, log_id);
}

KademliaTable::Peers KademliaTable::FindPeer(Address const &address, uint64_t log_id,
                                             bool scan_left, bool scan_right)
{
  FETCH_LOCK(mutex_);

  // Computing the Kademlia distance and the
  // corresponding bucket.
  auto kam_address = KademliaAddress::Create(address);
  return FindPeerInternal(kam_address, log_id, scan_left, scan_right);
}

KademliaTable::Peers KademliaTable::FindPeerByHammingInternal(KademliaAddress const &kam_address,
                                                              uint64_t hamming_id, bool scan_left,
                                                              bool scan_right)
{
  // TODO: Merge with other function.
  // Checking that we are within bounds
  if (hamming_id > KADEMLIA_MAX_ID_BITS)
  {
    return {};
  }

  // Creating initial list
  std::unordered_map<Address, PeerInfo> results;
  for (auto &p : by_hamming_[hamming_id].peers)
  {
    results.insert({p->address, *p});
  }

  // Preparing to search nearby buckets in case there is not
  // enough peers in the current bucket.
  auto left            = hamming_id;
  auto right           = hamming_id;
  bool need_more_peers = results.size() < kademlia_max_peers_per_bucket_;

  // Checking if we need to go to other buckets to add more
  // peers.
  while (need_more_peers)
  {
    need_more_peers = false;

    // Searhcing the buckets left of the main bucket
    if (scan_left && (left != 0))
    {
      left -= 1;
      for (auto const &p : by_hamming_[left].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket_;
    }

    // And the ones right of the main bucket.
    if (scan_right && (right < KADEMLIA_MAX_ID_BITS))
    {
      right += 1;

      for (auto const &p : by_hamming_[right].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket_;
    }
  }

  // Worst cast size of the return is at this point
  // 3 * kademlia_max_peers_per_bucket_ - 1. We now
  // trim and sort according to distance
  assert(results.size() < 3 * kademlia_max_peers_per_bucket_);
  Peers ret;
  for (auto &peer : results)
  {
    peer.second.distance = GetKademliaDistance(peer.second.kademlia_address, kam_address);
    ret.push_back(peer.second);
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

KademliaTable::Peers KademliaTable::FindPeerByHamming(Address const &address)
{
  FETCH_LOCK(mutex_);
  auto kam_address = KademliaAddress::Create(address);
  auto dist        = GetKademliaDistance(own_address_, kam_address);
  auto hamming_id  = GetBucketByLogarithm(dist);

  return FindPeerByHammingInternal(kam_address, hamming_id);
}

KademliaTable::Peers KademliaTable::FindPeerByHamming(Address const &address, uint64_t hamming_id,
                                                      bool scan_left, bool scan_right)
{
  FETCH_LOCK(mutex_);
  auto kam_address = KademliaAddress::Create(address);
  return FindPeerInternal(kam_address, hamming_id, scan_left, scan_right);
}

void KademliaTable::ReportLiveliness(Address const &address, Address const &reporter,
                                     PeerInfo const &info)
{
  FETCH_LOCK(mutex_);

  auto other      = KademliaAddress::Create(address);
  auto dist       = GetKademliaDistance(own_address_, other);
  auto log_id     = GetBucketByLogarithm(dist);
  auto hamming_id = GetBucketByHamming(dist);

  assert(log_id <= KADEMLIA_MAX_ID_BITS);

  // Fetching the log bucket and finding the peer if it exists in it
  auto &log_bucket = by_logarithm_[log_id];
  auto  log_it     = log_bucket.peers.begin();
  for (; log_it != log_bucket.peers.end(); ++log_it)
  {
    if (address == (*log_it)->address)
    {
      log_bucket.peers.erase(log_it);
      break;
    }
  }

  // Fetching the hamming bucket and finding the peer if it exists in it
  auto &hamming_bucket = by_hamming_[hamming_id];
  auto  hamming_it     = hamming_bucket.peers.begin();
  for (; hamming_it != hamming_bucket.peers.end(); ++hamming_it)
  {
    if (address == (*hamming_it)->address)
    {
      hamming_bucket.peers.erase(hamming_it);
      break;
    }
  }

  // Peer is already known but not in any
  // log_bucket.
  PeerInfoPtr peerinfo;
  auto        it = know_peers_.find(address);
  if (it != know_peers_.end())
  {
    peerinfo = it->second;
  }
  else
  {
    // Discovered a new peer.
    peerinfo                   = std::make_shared<PeerInfo>(info);
    peerinfo->kademlia_address = other;
    peerinfo->distance         = dist;
    peerinfo->address          = address;

    // Ensures that peer information persists over time
    // even if the peer disappears from the log_bucket.
    know_peers_[address] = peerinfo;
  }

  // Updating activity information
  // TODO: This last part is wrong
  peerinfo->last_reporter = reporter;
  peerinfo->verified      = true;
  peerinfo->message_count += 1;
  // TODO: peerinfo.last_activity

  // Updating buckets
  log_bucket.peers.insert(peerinfo);
  hamming_bucket.peers.insert(peerinfo);

  // Updating own bucket
  if (log_id < first_non_empty_bucket_)
  {
    first_non_empty_bucket_ = log_id;
  }
  // Triming the list
  // TODO: move to after pinging
  /*
  while (bucket.peers.size() > kademlia_max_peers_per_bucket_)
  {
    // TODO: Pop
    //    bucket.peers.pop_front();
  }
  */
}

void KademliaTable::ReportExistence(PeerInfo const &info, Address const &reporter)
{
  //  ReportLiveliness(info.address, info);
  //  return;
  FETCH_LOCK(mutex_);

  auto other      = KademliaAddress::Create(info.address);
  auto dist       = GetKademliaDistance(own_address_, other);
  auto log_id     = GetBucketByLogarithm(dist);
  auto hamming_id = GetBucketByHamming(dist);

  assert(log_id <= KADEMLIA_MAX_ID_BITS);

  // Do nothing if we already know about the existence
  auto it = know_peers_.find(info.address);

  // Fetching the bucket and finding the peer if it is in the
  // bucket
  if (it == know_peers_.end())
  {
    auto &      log_bucket  = by_logarithm_[log_id];
    auto &      ham_bucket  = by_hamming_[hamming_id];
    PeerInfoPtr peerinfo    = std::make_shared<PeerInfo>(info);
    peerinfo->verified      = false;
    peerinfo->last_reporter = reporter;

    know_peers_[info.address] = peerinfo;

    //
    if (log_bucket.peers.size() < kademlia_max_peers_per_bucket_)
    {
      log_bucket.peers.insert(peerinfo);
    }

    if (ham_bucket.peers.size() < kademlia_max_peers_per_bucket_)
    {
      ham_bucket.peers.insert(peerinfo);
    }
  }
  else
  {
    // Updating
    // TODO: Update to time stamped certificiates
    if (!info.uri.empty())
    {
      it->second->uri = info.uri;
    }
    it->second->last_reporter = reporter;
  }

  // Updating own bucket
  if (log_id < first_non_empty_bucket_)
  {
    first_non_empty_bucket_ = log_id;
  }
}

KademliaTable::PeerInfoPtr KademliaTable::GetPeerDetails(Address const &address)
{
  auto it = know_peers_.find(address);
  if (it == know_peers_.end())
  {
    return nullptr;
  }

  return it->second;
}

void KademliaTable::ReportFailure(Address const & /*address*/, Address const & /*reporter*/)
{}

}  // namespace muddle
}  // namespace fetch