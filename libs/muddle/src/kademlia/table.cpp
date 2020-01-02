//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/sha1.hpp"
#include "kademlia/table.hpp"
#include "muddle/network_id.hpp"

namespace fetch {
namespace muddle {
namespace {

std::string GenerateLoggingName(NetworkId const &network)
{
  return "KademliaTable:" + network.ToString();
}

}  // namespace

KademliaTable::KademliaTable(Address const &own_address, NetworkId const &network)
  : logging_name_{GenerateLoggingName(network)}
  , own_address_{own_address}
  , own_kad_address_{KademliaAddress::Create(own_address)}
{}

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
  {
    FETCH_LOCK(peer_info_mutex_);
    for (auto &p : by_logarithm_[log_id].peers)
    {
      results.insert({p->address, *p});
    }
  }
  // Preparing to search nearby buckets in case there is not
  // enough peers in the current bucket.
  auto left            = log_id;
  auto right           = log_id;
  bool need_more_peers = results.size() < kademlia_max_peers_per_bucket();

  // Checking if we need to go to other buckets to add more
  // peers.
  while (need_more_peers)
  {
    need_more_peers = false;

    // Searhcing the buckets left of the main bucket
    if (scan_left && (left != 0))
    {
      FETCH_LOCK(peer_info_mutex_);
      left -= 1;
      for (auto const &p : by_logarithm_[left].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket();
    }

    // And the ones right of the main bucket.
    if (scan_right && (right < KADEMLIA_MAX_ID_BITS))
    {
      FETCH_LOCK(peer_info_mutex_);
      right += 1;

      for (auto const &p : by_logarithm_[right].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket();
    }
  }

  // Worst cast size of the return is at this point
  // 3 * kademlia_max_peers_per_bucket_ - 1. We now
  // trim and sort according to distance
  assert(results.size() < 3 * kademlia_max_peers_per_bucket());
  Peers ret;
  for (auto &peer : results)
  {
    peer.second.distance = GetKademliaDistance(peer.second.kademlia_address, kam_address);
    ret.push_back(peer.second);
  }

  // Sorting according to distance
  std::sort(ret.begin(), ret.end());

  // Trimming the list
  while (ret.size() > kademlia_max_peers_per_bucket())
  {
    ret.pop_back();
  }

  return ret;
}

KademliaTable::Peers KademliaTable::FindPeer(Address const &address)
{
  // Computing the Kademlia distance and the
  // corresponding bucket.
  auto kam_address = KademliaAddress::Create(address);
  auto dist        = GetKademliaDistance(own_kad_address_, kam_address);
  auto log_id      = Bucket::IdByLogarithm(dist);

  return FindPeerInternal(kam_address, log_id);
}

KademliaTable::Peers KademliaTable::FindPeer(Address const &address, uint64_t log_id,
                                             bool scan_left, bool scan_right)
{
  // Computing the Kademlia distance and the
  // corresponding bucket.
  auto kam_address = KademliaAddress::Create(address);
  return FindPeerInternal(kam_address, log_id, scan_left, scan_right);
}

KademliaTable::Peers KademliaTable::FindPeerByHammingInternal(KademliaAddress const &kam_address,
                                                              uint64_t hamming_id, bool scan_left,
                                                              bool scan_right)
{
  // TODO(tfr): Merge with other function.
  // Checking that we are within bounds
  if (hamming_id > KADEMLIA_MAX_ID_BITS)
  {
    return {};
  }

  // Creating initial list
  std::unordered_map<Address, PeerInfo> results;
  {
    FETCH_LOCK(peer_info_mutex_);

    for (auto &p : by_hamming_[hamming_id].peers)
    {
      results.insert({p->address, *p});
    }
  }
  // Preparing to search nearby buckets in case there is not
  // enough peers in the current bucket.
  auto left            = hamming_id;
  auto right           = hamming_id;
  bool need_more_peers = results.size() < kademlia_max_peers_per_bucket();

  // Checking if we need to go to other buckets to add more
  // peers.
  while (need_more_peers)
  {
    need_more_peers = false;

    // Searhcing the buckets left of the main bucket
    if (scan_left && (left != 0))
    {
      left -= 1;
      FETCH_LOCK(peer_info_mutex_);
      for (auto const &p : by_hamming_[left].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket();
    }

    // And the ones right of the main bucket.
    if (scan_right && (right < KADEMLIA_MAX_ID_BITS))
    {
      right += 1;
      FETCH_LOCK(peer_info_mutex_);
      for (auto const &p : by_hamming_[right].peers)
      {
        results.insert({p->address, *p});
      }

      need_more_peers = results.size() < kademlia_max_peers_per_bucket();
    }
  }

  // Worst cast size of the return is at this point
  // 3 * kademlia_max_peers_per_bucket_ - 1. We now
  // trim and sort according to distance
  assert(results.size() < 3 * kademlia_max_peers_per_bucket());
  Peers ret;
  for (auto &peer : results)
  {
    peer.second.distance = GetKademliaDistance(peer.second.kademlia_address, kam_address);
    ret.push_back(peer.second);
  }

  // Sorting according to distance
  std::sort(ret.begin(), ret.end());

  // Trimming the list
  while (ret.size() > kademlia_max_peers_per_bucket())
  {
    ret.pop_back();
  }

  return ret;
}

KademliaTable::Peers KademliaTable::FindPeerByHamming(Address const &address)
{
  auto kam_address = KademliaAddress::Create(address);
  auto dist        = GetKademliaDistance(own_kad_address_, kam_address);
  auto hamming_id  = Bucket::IdByHamming(dist);

  return FindPeerByHammingInternal(kam_address, hamming_id);
}

KademliaTable::Peers KademliaTable::FindPeerByHamming(Address const &address, uint64_t hamming_id,
                                                      bool scan_left, bool scan_right)
{
  auto kam_address = KademliaAddress::Create(address);
  return FindPeerInternal(kam_address, hamming_id, scan_left, scan_right);
}

void KademliaTable::ReportSuccessfulConnectAttempt(Uri const &uri)
{
  FETCH_LOCK(peer_info_mutex_);
  auto it = known_uris_.find(uri);
  if (it == known_uris_.end())
  {
    PeerInfoPtr info = std::make_shared<PeerInfo>();
    info->uri        = uri;
    known_uris_[uri] = info;
    it               = known_uris_.find(uri);
  }

  it->second->failed_attempts  = 0;
  it->second->connection_start = Clock::now();
  ++(it->second->connection_attempts);
  ++(it->second->connections);

  // In case we loose connectivity we allow to immediately reconmnect.
  it->second->earliest_next_attempt = Clock::now();
}

void KademliaTable::ReportFailedConnectAttempt(Uri const &uri)
{
  FETCH_LOCK(peer_info_mutex_);
  auto it = known_uris_.find(uri);
  if (it == known_uris_.end())
  {
    // TODO(tfr): Consider creating the entry if it does not exist
    return;
  }

  ++(it->second->connection_attempts);
  ++(it->second->failed_attempts);
  // Exponentially killing the likelihood that we will connect again if failed
  it->second->earliest_next_attempt =
      Clock::now() + std::chrono::seconds(10 * (1 << it->second->failed_attempts));
}

void KademliaTable::ReportLeaving(Uri const &uri)
{
  FETCH_LOCK(peer_info_mutex_);
  auto it = known_uris_.find(uri);
  if (it == known_uris_.end())
  {
    return;
  }
  --(it->second->connections);
}

void KademliaTable::ReportLiveliness(Address const &address, Address const &reporter,
                                     PeerInfo const &info)
{
  // We never register our own address
  if (address == own_address())
  {
    return;
  }

  auto other      = KademliaAddress::Create(address);
  auto dist       = GetKademliaDistance(own_kademlia_address(), other);
  auto log_id     = Bucket::IdByLogarithm(dist);
  auto hamming_id = Bucket::IdByHamming(dist);

  assert(log_id <= KADEMLIA_MAX_ID_BITS);

  // Fetching the log bucket and finding the peer if it exists in it
  {
    FETCH_LOCK(peer_info_mutex_);

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
  }

  // Fetching the hamming bucket and finding the peer if it exists in it
  {
    FETCH_LOCK(peer_info_mutex_);
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
  }

  // Peer is already known but not in any
  // log_bucket.
  {
    FETCH_LOCK(peer_info_mutex_);
    PeerInfoPtr peerinfo;

    auto &log_bucket     = by_logarithm_[log_id];
    auto &hamming_bucket = by_hamming_[hamming_id];

    auto it = known_peers_.find(address);
    if (it != known_peers_.end())
    {
      peerinfo = it->second;
    }
    else
    {
      // Discovered a new peer.
      peerinfo                   = std::make_shared<PeerInfo>(info);
      peerinfo->kademlia_address = other;
      peerinfo->distance         = dist;
      peerinfo->address          = address.Copy();

      // Ensures that peer information persists over time
      // even if the peer disappears from the log_bucket.
      known_peers_[address]      = peerinfo;
      known_uris_[peerinfo->uri] = peerinfo;
    }

    // Updating activity information
    // TODO(tfr): This last part is wrong
    peerinfo->last_reporter = reporter;
    peerinfo->verified      = true;
    peerinfo->message_count += 1;
    // TODO(tfr): peerinfo.last_activity

    // Updating buckets
    log_bucket.peers.insert(peerinfo);
    hamming_bucket.peers.insert(peerinfo);
  }

  // Updating own bucket
  {
    FETCH_LOCK(core_mutex_);

    if (log_id < first_non_empty_bucket_)
    {
      first_non_empty_bucket_ = log_id;
    }
  }
  // Triming the list
  // TODO(tfr): move to after pinging
  /*
  while (bucket.peers.size() > kademlia_max_peers_per_bucket_)
  {
    // TODO(tfr): Pop
    //    bucket.peers.pop_front();
  }
  */
}

void KademliaTable::ReportExistence(PeerInfo const &info, Address const &reporter)
{
  FETCH_LOCK(peer_info_mutex_);

  if (info.address == own_address())
  {
    return;
  }

  auto other      = KademliaAddress::Create(info.address);
  auto dist       = GetKademliaDistance(own_kad_address_, other);
  auto log_id     = Bucket::IdByLogarithm(dist);
  auto hamming_id = Bucket::IdByHamming(dist);

  assert(log_id <= KADEMLIA_MAX_ID_BITS);

  // Do nothing if we already know about the existence
  auto it = known_peers_.find(info.address);

  // Fetching the bucket and finding the peer if it is in the
  // bucket
  if (it == known_peers_.end())
  {
    auto &      log_bucket  = by_logarithm_[log_id];
    auto &      ham_bucket  = by_hamming_[hamming_id];
    PeerInfoPtr peerinfo    = std::make_shared<PeerInfo>(info);
    peerinfo->verified      = false;
    peerinfo->last_reporter = reporter;

    known_peers_[info.address] = peerinfo;

    //
    if (log_bucket.peers.size() < kademlia_max_peers_per_bucket_)
    {
      log_bucket.peers.insert(peerinfo);
    }

    if (ham_bucket.peers.size() < kademlia_max_peers_per_bucket_)
    {
      ham_bucket.peers.insert(peerinfo);
    }

    if (peerinfo->uri.IsMuddleAddress())
    {
      known_uris_[peerinfo->uri] = peerinfo;
    }
  }
  else
  {
    // Updating
    // TODO(tfr): Update to time stamped certificiates
    if (info.uri.IsMuddleAddress())
    {
      it->second->uri              = info.uri;
      known_uris_[it->second->uri] = it->second;
    }
    it->second->last_reporter = reporter;
  }

  // Updating own bucket
  {
    FETCH_LOCK(core_mutex_);
    if (log_id < first_non_empty_bucket_)
    {
      first_non_empty_bucket_ = log_id;
    }
  }
}

bool KademliaTable::HasPeerDetails(Address const &address)
{
  FETCH_LOCK(peer_info_mutex_);

  auto it = known_peers_.find(address);
  return (it != known_peers_.end());
}

PeerInfo KademliaTable::GetPeerDetails(Address const &address)
{
  FETCH_LOCK(peer_info_mutex_);

  auto it = known_peers_.find(address);
  if (it == known_peers_.end())
  {
    return {};
  }

  return *(it->second);
}

std::size_t KademliaTable::size() const
{
  FETCH_LOCK(peer_info_mutex_);
  return known_peers_.size();
}

KademliaTable::Uri KademliaTable::GetUri(Address const &address)
{
  FETCH_LOCK(peer_info_mutex_);
  auto it = known_peers_.find(address);
  if (it == known_peers_.end())
  {
    return {};
  }
  return it->second->uri;
}

std::size_t KademliaTable::active_buckets() const
{
  FETCH_LOCK(peer_info_mutex_);

  std::size_t ret{0};
  for (auto &b : by_logarithm_)
  {
    ret += static_cast<std::size_t>(!b.peers.empty());
  }
  return ret;
}

void KademliaTable::SetCacheFile(std::string const &filename, bool load)
{
  {
    FETCH_LOCK(core_mutex_);
    filename_ = filename;
  }
  if (load)
  {
    Load();
  }
}

void KademliaTable::Load()
{
  std::fstream stream(filename(), std::ios::in | std::ios::binary);
  if (!stream)
  {

    return;
  }

  // Loading buffer
  byte_array::ConstByteArray buffer{stream};

  // Deserializing
  try
  {
    serializers::LargeObjectSerializeHelper serializer(buffer);
    serializer >> *this;
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_ERROR("KademliaTable", "Failed loading the peer table: ", e.what());
  }
}

void KademliaTable::Dump()
{
  if (filename().empty())
  {
    return;
  }

  std::fstream stream(filename_, std::ios::out | std::ios::binary | std::ios::trunc);

  // Dumping table to file.
  if (stream)
  {
    serializers::LargeObjectSerializeHelper serializer{};

    FETCH_LOG_DEBUG("KademliaTable", "Dumping table.");
    serializer << *this;

    auto buffer = serializer.data();
    stream << buffer;

    stream.close();
  }
}

void KademliaTable::ClearDesired()
{
  FETCH_LOCK(desired_mutex_);
  desired_connection_expiry_.clear();
  desired_uri_expiry_.clear();
  desired_peers_.clear();
  desired_uris_.clear();
}

void KademliaTable::TrimDesiredPeers()
{
  FETCH_LOCK(desired_mutex_);
  // Trimming for expired connections
  auto                                   now = Clock::now();
  std::unordered_map<Address, Timepoint> new_expiry;
  for (auto const &item : desired_connection_expiry_)
  {

    // Keeping those which are still not expired
    if (item.second > now)
    {
      new_expiry.emplace(item);
    }
    else if (item.second < now)
    {
      // Deleting peers which has expired
      auto it = desired_peers_.find(item.first);
      if (it != desired_peers_.end())
      {
        desired_peers_.erase(it);
      }
    }
  }
  std::swap(desired_connection_expiry_, new_expiry);

  // Trimming URIs
  std::unordered_map<Uri, Timepoint> new_uri_expiry;
  for (auto const &item : desired_uri_expiry_)
  {
    // Keeping those which are still not expired
    if (item.second > now)
    {
      new_uri_expiry.emplace(item);
    }
    else if (item.second < now)
    {
      // Deleting peers which has expired
      auto it = desired_uris_.find(item.first);
      if (it != desired_uris_.end())
      {
        desired_uris_.erase(it);
      }
    }
  }
  std::swap(desired_uri_expiry_, new_uri_expiry);
}

void KademliaTable::ConvertDesiredUrisToAddresses()
{
  FETCH_LOCK(desired_mutex_);
  std::unordered_set<Uri> new_uris;
  for (auto const &uri : desired_uris_)
  {
    if (IsConnectedToUri(uri))
    {
      auto address = GetAddressFromUri(uri);

      // Moving expiry time accross based on address
      auto expit = desired_uri_expiry_.find(uri);
      if (expit != desired_uri_expiry_.end())
      {
        desired_connection_expiry_[address] = expit->second;
        desired_uri_expiry_.erase(expit);
      }

      // Switching to address based desired peer
      if (address != own_address())
      {
        desired_peers_.insert(std::move(address));
      }
    }
    else
    {
      new_uris.insert(uri);
    }
  }
  std::swap(new_uris, desired_uris_);
}

std::unordered_set<KademliaTable::Uri> KademliaTable::desired_uris() const
{
  FETCH_LOCK(desired_mutex_);
  return desired_uris_;
}

KademliaTable::AddressSet KademliaTable::desired_peers() const
{
  FETCH_LOCK(desired_mutex_);
  return desired_peers_;
}

void KademliaTable::AddDesiredPeer(Address const &address, network::Peer const &hint,
                                   Duration const &expiry)
{
  PeerInfo info;
  {
    FETCH_LOCK(desired_mutex_);
    FETCH_LOCK(peer_info_mutex_);

    auto it = desired_connection_expiry_.find(address);
    if (it == desired_connection_expiry_.end())
    {
      desired_connection_expiry_.emplace(address, Clock::now() + expiry);
    }
    else
    {
      it->second = std::max(Clock::now() + expiry, it->second);
    }

    info.address = address.Copy();
    info.uri.Parse(hint.ToUri());

    // Deleting information that is contracdictary
    auto it2 = known_peers_.find(address);
    if (it2 != known_peers_.end())
    {
      if (it2->second->uri != info.uri)
      {
        known_peers_.erase(it2);
      }
    }

    auto it3 = known_uris_.find(info.uri);
    if (it3 != known_uris_.end())
    {
      if (it3->second->address != info.address)
      {
        known_uris_.erase(it3);
      }
    }
  }

  // Reporting
  ReportExistence(info, own_address());

  // Note we might previously have erased it2
  bool add_address{false};
  {
    FETCH_LOCK(peer_info_mutex_);
    auto it2    = known_peers_.find(address);
    add_address = (it2 != known_peers_.end());
  }

  if (!address.empty() && add_address)
  {
    AddDesiredPeerInternal(address, expiry);
  }
  else
  {
    AddDesiredPeerInternal(info.uri, expiry);
  }
}

void KademliaTable::AddDesiredPeer(Address const &address, Duration const &expiry)
{
  AddDesiredPeerInternal(address, expiry);
}

void KademliaTable::AddDesiredPeer(Uri const &uri, Duration const &expiry)
{
  AddDesiredPeerInternal(uri, expiry);
}

void KademliaTable::AddDesiredPeerInternal(Address const &address, Duration const &expiry)
{
  FETCH_LOCK(desired_mutex_);
  auto it = desired_connection_expiry_.find(address);
  if (it == desired_connection_expiry_.end())
  {
    desired_connection_expiry_.emplace(address, Clock::now() + expiry);
  }
  else
  {
    it->second = std::max(Clock::now() + expiry, it->second);
  }

  desired_peers_.insert(address);
}

void KademliaTable::AddDesiredPeerInternal(Uri const &uri, Duration const &expiry)
{
  FETCH_LOCK(desired_mutex_);
  // TODO(LDGR-641): Will not work if spammed with URIs
  desired_uris_.insert(uri);
  desired_uri_expiry_.emplace(uri, Clock::now() + expiry);
}

void KademliaTable::RemoveDesiredPeer(Address const &address)
{
  FETCH_LOCK(desired_mutex_);
  desired_peers_.erase(address);
  desired_connection_expiry_.erase(address);
}

bool KademliaTable::HasUri(Uri const &uri) const
{
  FETCH_LOCK(peer_info_mutex_);
  auto it = known_uris_.find(uri);
  return (it != known_uris_.end());
}

bool KademliaTable::IsConnectedToUri(Uri const &uri) const
{
  FETCH_LOCK(peer_info_mutex_);
  auto it = known_uris_.find(uri);
  return (it != known_uris_.end()) && (it->second->connections > 0);
}

KademliaTable::Address KademliaTable::GetAddressFromUri(Uri const &uri) const
{
  FETCH_LOCK(peer_info_mutex_);
  auto it = known_uris_.find(uri);
  if (it == known_uris_.end())
  {
    return {};
  }
  return it->second->address;
}

void KademliaTable::ReportFailure(Address const & /*address*/, Address const & /*reporter*/)
{}

}  // namespace muddle
}  // namespace fetch
