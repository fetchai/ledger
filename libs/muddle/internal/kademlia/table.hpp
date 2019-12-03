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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/sha1.hpp"
#include "kademlia/bucket.hpp"
#include "kademlia/primitives.hpp"
#include "muddle/packet.hpp"

namespace fetch {
namespace muddle {

class KademliaTable
{
public:
  static constexpr uint64_t KADEMLIA_MAX_ID_BITS = KademliaAddress::KADEMLIA_MAX_ID_BITS;

  using Buckets        = std::array<Bucket, KADEMLIA_MAX_ID_BITS + 1>;
  using Peers          = std::deque<PeerInfo>;
  using Address        = Packet::Address;
  using PeerInfoPtr    = std::shared_ptr<PeerInfo>;
  using PeerMap        = std::unordered_map<Address, PeerInfoPtr>;
  using Uri            = network::Uri;
  using UriToPeerMap   = std::unordered_map<Uri, PeerInfoPtr>;
  using Port           = uint16_t;
  using PortList       = std::vector<Port>;
  using ConstByteArray = byte_array::ConstByteArray;

  explicit KademliaTable(Address const &own_address);

  /// Construtors & destructors
  /// @{
  KademliaTable(KademliaTable const &) = delete;
  KademliaTable(KademliaTable &&)      = delete;
  ~KademliaTable()                     = default;
  /// @}

  /// Operators
  /// @{
  KademliaTable &operator=(KademliaTable const &) = delete;
  KademliaTable &operator=(KademliaTable &&) = delete;
  /// @}

  /// Kademlia
  /// @{
  ConstByteArray Ping(Address const &address, PortList port);
  Peers          FindPeer(Address const &address);
  Peers          FindPeer(Address const &address, uint64_t log_id, bool scan_left = true,
                          bool scan_right = true);
  Peers          FindPeerByHamming(Address const &address);
  Peers FindPeerByHamming(Address const &address, uint64_t hamming_id, bool scan_left = true,
                          bool scan_right = true);
  void ReportLiveliness(Address const &address, Address const &reporter, PeerInfo const &info = {});
  void ReportExistence(PeerInfo const &info, Address const &reporter);
  void ReportFailure(Address const &address, Address const &reporter);
  PeerInfoPtr GetPeerDetails(Address const &address);

  bool HasUri(Uri const &uri) const
  {
    auto it = known_uris_.find(uri);
    return (it != known_uris_.end()) && (!it->second->address.empty());
  }

  Address GetAddressFromUri(Uri const &uri) const
  {
    auto it = known_uris_.find(uri);
    if (it == known_uris_.end())
    {
      return {};
    }
    return it->second->address;
  }
  /// @}

  /// For connection maintenance
  /// @{
  Peers ProposePermanentConnections() const;
  /// @}

  std::size_t size() const
  {
    FETCH_LOCK(mutex_);
    return know_peers_.size();
  }

  Uri GetUri(Address const &address)
  {
    auto it = know_peers_.find(address);
    if (it == know_peers_.end())
    {
      return {};
    }
    return it->second->uri;
  }

  std::size_t active_buckets() const
  {
    std::size_t ret{0};
    for (auto &b : by_logarithm_)
    {
      ret += static_cast<std::size_t>(!b.peers.empty());
    }
    return ret;
  }

  uint64_t first_non_empty_bucket() const
  {
    return first_non_empty_bucket_;
  }

private:
  Peers FindPeerInternal(KademliaAddress const &kam_address, uint64_t log_id, bool scan_left = true,
                         bool scan_right = true);
  Peers FindPeerByHammingInternal(KademliaAddress const &kam_address, uint64_t hamming_id,
                                  bool scan_left = true, bool scan_right = true);
  mutable std::mutex mutex_;
  KademliaAddress    own_address_;
  Buckets            by_logarithm_;
  Buckets            by_hamming_;
  PeerMap            know_peers_;
  UriToPeerMap       known_uris_;

  uint64_t first_non_empty_bucket_{KADEMLIA_MAX_ID_BITS};
  uint64_t kademlia_max_peers_per_bucket_{20};
};

}  // namespace muddle
}  // namespace fetch
