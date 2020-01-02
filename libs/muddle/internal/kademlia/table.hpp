#pragma once
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
#include "core/serializers/main_serializer.hpp"
#include "crypto/sha1.hpp"
#include "kademlia/bucket.hpp"
#include "kademlia/primitives.hpp"
#include "moment/clock_interfaces.hpp"
#include "muddle/packet.hpp"

#include <fstream>

namespace fetch {
namespace muddle {

class NetworkId;

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
  using AddressSet     = std::unordered_set<Address>;

  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  KademliaTable(Address const &own_address, NetworkId const &network);

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
  Peers       FindPeerByHamming(Address const &address, uint64_t hamming_id, bool scan_left = true,
                                bool scan_right = true);
  bool        HasPeerDetails(Address const &address);
  PeerInfo    GetPeerDetails(Address const &address);
  bool        HasUri(Uri const &uri) const;
  bool        IsConnectedToUri(Uri const &uri) const;
  Address     GetAddressFromUri(Uri const &uri) const;
  std::size_t size() const;
  Uri         GetUri(Address const &address);
  std::size_t active_buckets() const;
  /// @}

  /// Reporting
  /// @{
  void ReportSuccessfulConnectAttempt(Uri const &uri);
  void ReportFailedConnectAttempt(Uri const &uri);
  void ReportLeaving(Uri const &uri);
  void ReportLiveliness(Address const &address, Address const &reporter, PeerInfo const &info = {});
  void ReportExistence(PeerInfo const &info, Address const &reporter);
  void ReportFailure(Address const &address, Address const &reporter);
  /// @}

  /// Storage of peer table.
  /// @{
  void Dump();
  void Load();
  void SetCacheFile(std::string const &filename, bool load = true);
  /// @}

  Address own_address() const
  {
    FETCH_LOCK(core_mutex_);
    return own_address_;
  }

  KademliaAddress own_kademlia_address() const
  {
    FETCH_LOCK(core_mutex_);
    return own_kad_address_;
  }
  uint64_t first_non_empty_bucket() const
  {
    FETCH_LOCK(core_mutex_);
    return first_non_empty_bucket_;
  }
  uint64_t kademlia_max_peers_per_bucket() const
  {
    FETCH_LOCK(core_mutex_);
    return kademlia_max_peers_per_bucket_;
  }
  std::string filename() const
  {
    FETCH_LOCK(core_mutex_);
    return filename_;
  }

protected:
  friend class PeerTracker;

  /// Methods to manage desired peers
  /// @{
  void                    ClearDesired();
  void                    TrimDesiredPeers();
  void                    ConvertDesiredUrisToAddresses();
  std::unordered_set<Uri> desired_uris() const;
  AddressSet              desired_peers() const;
  void AddDesiredPeer(Address const &address, network::Peer const &hint, Duration const &expiry);
  void AddDesiredPeer(Address const &address, Duration const &expiry);
  void AddDesiredPeer(Uri const &uri, Duration const &expiry);
  void RemoveDesiredPeer(Address const &address);
  /// @}

private:
  void AddDesiredPeerInternal(Address const &address, Duration const &expiry);
  void AddDesiredPeerInternal(Uri const &uri, Duration const &expiry);

  Peers FindPeerInternal(KademliaAddress const &kam_address, uint64_t log_id, bool scan_left = true,
                         bool scan_right = true);
  Peers FindPeerByHammingInternal(KademliaAddress const &kam_address, uint64_t hamming_id,
                                  bool scan_left = true, bool scan_right = true);
  std::string const logging_name_;

  // The mutex locking order should be in the order in which
  // they are listed below, with core_mutex_ being the first
  // and desired_mutex_ being the last

  // Core variables
  /// @{
  mutable Mutex         core_mutex_;
  Address               own_address_;
  KademliaAddress       own_kad_address_;
  uint64_t              first_non_empty_bucket_{KADEMLIA_MAX_ID_BITS};
  std::atomic<uint64_t> kademlia_max_peers_per_bucket_{20};
  std::string           filename_{};
  /// @}

  /// @{
  mutable Mutex peer_info_mutex_;
  Buckets       by_logarithm_;
  Buckets       by_hamming_;
  PeerMap       known_peers_;
  UriToPeerMap  known_uris_;
  /// @}

  /// User defined connections
  /// @{
  mutable Mutex desired_mutex_;  ///< Use to protect desired peer variables
                                 /// peers to avoid causing a deadlock
  std::unordered_map<Address, Timepoint> desired_connection_expiry_;
  std::unordered_map<Uri, Timepoint>     desired_uri_expiry_;
  AddressSet                             desired_peers_;
  std::unordered_set<Uri>                desired_uris_;
  /// @}

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

}  // namespace muddle

namespace serializers {

template <typename D>
struct MapSerializer<muddle::KademliaTable, D>
{
public:
  using Type       = muddle::KademliaTable;
  using DriverType = D;

  static uint8_t const KNOWN_PEERS       = 1;
  static uint8_t const CONNECTION_EXPIRY = 2;
  static uint8_t const DESIRED_EXPIRY    = 3;
  static uint8_t const DESIRED_PEERS     = 4;
  static uint8_t const DESIRED_URIS      = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(5);

    // Convering the known peers into a vector and only store
    // those that has a valid URI.
    FETCH_LOCK(item.peer_info_mutex_);
    std::vector<muddle::PeerInfo> peers;
    for (auto &p : item.known_peers_)
    {
      if (p.second)
      {
        if (p.second->uri.IsValid())
        {
          peers.push_back(*p.second);
        }
      }
    }

    FETCH_LOCK(item.desired_mutex_);
    map.Append(KNOWN_PEERS, peers);
    map.Append(CONNECTION_EXPIRY, item.desired_connection_expiry_);
    map.Append(DESIRED_EXPIRY, item.desired_uri_expiry_);
    map.Append(DESIRED_PEERS, item.desired_peers_);
    map.Append(DESIRED_URIS, item.desired_uris_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    std::vector<muddle::PeerInfo> peers;
    // We reconstruct the table from the peer list.
    // This invalidates all information about the liveness
    // of the peer which would be needed any way on a fresh restart
    // This is also needed to ensure that pointers are constructed
    // correctly
    map.ExpectKeyGetValue(KNOWN_PEERS, peers);
    for (auto &p : peers)
    {
      item.ReportExistence(p, p.last_reporter);
    }

    FETCH_LOCK(item.peer_info_mutex_);
    FETCH_LOCK(item.desired_mutex_);
    map.ExpectKeyGetValue(CONNECTION_EXPIRY, item.desired_connection_expiry_);
    map.ExpectKeyGetValue(DESIRED_EXPIRY, item.desired_uri_expiry_);
    map.ExpectKeyGetValue(DESIRED_PEERS, item.desired_peers_);
    map.ExpectKeyGetValue(DESIRED_URIS, item.desired_uris_);
  }
};
}  // namespace serializers
}  // namespace fetch
