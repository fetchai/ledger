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
#include "core/serializers/main_serializer.hpp"
#include "crypto/sha1.hpp"
#include "kademlia/bucket.hpp"
#include "kademlia/primitives.hpp"
#include "moment/clock_interfaces.hpp"
#include "muddle/packet.hpp"

#include <fstream>

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
  using AddressSet     = std::unordered_set<Address>;

  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

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
  bool        HasUri(Uri const &uri) const
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

  void SetCacheFile(std::string const &filename)
  {
    filename_ = filename;
    Load();
  }

  void Load()
  {
    std::fstream stream(filename_, std::ios::in | std::ios::binary);
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
      FETCH_LOG_ERROR("KademliaTable", "Failed loading the peer table.");
    }
  }

  void Dump()
  {
    if (filename_.empty())
    {
      return;
    }

    std::fstream stream(filename_, std::ios::out | std::ios::binary | std::ios::trunc);

    // Dumping table to file.
    if (stream)
    {
      serializers::LargeObjectSerializeHelper serializer{};

      FETCH_LOG_WARN("KademliaTable", "Dumping table.");
      serializer << *this;

      auto buffer = serializer.data();
      stream << buffer;

      stream.close();
    }
  }

protected:
  friend class PeerTracker;

  /// Methods to manage desired peers
  /// @{
  void ClearDesired()
  {
    connection_expiry_.clear();
    desired_uri_expiry_.clear();
    desired_peers_.clear();
    desired_uris_.clear();
  }

  void TrimDesiredPeers()
  {
    FETCH_LOCK(desired_mutex_);
    // Trimming for expired connections
    auto                                   now = Clock::now();
    std::unordered_map<Address, Timepoint> new_expiry;
    for (auto const &item : connection_expiry_)
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
    std::swap(connection_expiry_, new_expiry);

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

  void ConvertDesiredUrisToAddresses()
  {
    FETCH_LOCK(desired_mutex_);
    std::unordered_set<Uri> new_uris;
    for (auto const &uri : desired_uris_)
    {
      if (HasUri(uri))
      {
        auto address = GetAddressFromUri(uri);

        // Moving expiry time accross based on address
        auto expit = desired_uri_expiry_.find(uri);
        if (expit != desired_uri_expiry_.end())
        {
          connection_expiry_[address] = expit->second;
          desired_uri_expiry_.erase(expit);
        }

        // Switching to address based desired peer
        if (address != own_address_)
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

  std::unordered_set<Uri> desired_uris() const
  {
    FETCH_LOCK(desired_mutex_);
    return desired_uris_;
  }

  AddressSet desired_peers() const
  {
    FETCH_LOCK(desired_mutex_);
    return desired_peers_;
  }

  void AddDesiredPeer(Address const &address, Duration const &expiry)
  {
    FETCH_LOCK(desired_mutex_);

    auto it = connection_expiry_.find(address);
    if (it == connection_expiry_.end())
    {
      connection_expiry_.emplace(address, Clock::now() + expiry);
    }
    else
    {
      it->second = std::max(Clock::now() + expiry, it->second);
    }

    desired_peers_.insert(address);
  }

  void AddDesiredPeer(Address const &address, network::Peer const &hint, Duration const &expiry)
  {
    FETCH_LOCK(desired_mutex_);

    auto it = connection_expiry_.find(address);
    if (it == connection_expiry_.end())
    {
      connection_expiry_.emplace(address, Clock::now() + expiry);
    }
    else
    {
      it->second = std::max(Clock::now() + expiry, it->second);
    }

    if (!address.empty())
    {
      desired_peers_.insert(address);
    }

    PeerInfo info;
    info.address = address;
    info.uri.Parse(hint.ToUri());
    ReportExistence(info, own_address_);

    desired_uris_.insert(info.uri);
    desired_uri_expiry_.emplace(info.uri, Clock::now() + expiry);
  }

  void AddDesiredPeer(Uri const &uri, Duration const &expiry)
  {
    FETCH_LOCK(desired_mutex_);
    desired_uris_.insert(uri);
    desired_uri_expiry_.emplace(uri, Clock::now() + expiry);
  }

  void RemoveDesiredPeer(Address const &address)
  {
    FETCH_LOCK(desired_mutex_);
    desired_peers_.erase(address);
  }

  /// @}

private:
  Peers FindPeerInternal(KademliaAddress const &kam_address, uint64_t log_id, bool scan_left = true,
                         bool scan_right = true);
  Peers FindPeerByHammingInternal(KademliaAddress const &kam_address, uint64_t hamming_id,
                                  bool scan_left = true, bool scan_right = true);
  mutable std::mutex mutex_;
  Address            own_address_;
  KademliaAddress    own_kad_address_;
  Buckets            by_logarithm_;
  Buckets            by_hamming_;
  PeerMap            know_peers_;
  UriToPeerMap       known_uris_;

  uint64_t first_non_empty_bucket_{KADEMLIA_MAX_ID_BITS};
  uint64_t kademlia_max_peers_per_bucket_{20};

  /// User defined connections
  /// @{
  mutable std::mutex desired_mutex_;  ///< Use to protect desired peer variables
                                      /// peers to avoid causing a deadlock
  std::unordered_map<Address, Timepoint> connection_expiry_;
  std::unordered_map<Uri, Timepoint>     desired_uri_expiry_;
  AddressSet                             desired_peers_;
  std::unordered_set<Uri>                desired_uris_;
  /// @}

  /// Backup
  /// @{
  std::string filename_{};
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

  static uint8_t const BY_LOGARITHM      = 1;
  static uint8_t const BY_HAMMING        = 2;
  static uint8_t const KNOWN_PEERS       = 3;
  static uint8_t const KNOWN_URIS        = 4;
  static uint8_t const CONNECTION_EXPIRY = 5;
  static uint8_t const DESIRED_EXPIRY    = 6;
  static uint8_t const DESIRED_PEERS     = 7;
  static uint8_t const DESIRED_URIS      = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(8);

    map.Append(BY_LOGARITHM, item.by_logarithm_);
    map.Append(BY_HAMMING, item.by_hamming_);
    map.Append(KNOWN_PEERS, item.know_peers_);
    map.Append(KNOWN_URIS, item.known_uris_);
    map.Append(CONNECTION_EXPIRY, item.connection_expiry_);
    map.Append(DESIRED_EXPIRY, item.desired_uri_expiry_);
    map.Append(DESIRED_PEERS, item.desired_peers_);
    map.Append(DESIRED_URIS, item.desired_uris_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    map.ExpectKeyGetValue(BY_LOGARITHM, item.by_logarithm_);
    map.ExpectKeyGetValue(BY_HAMMING, item.by_hamming_);
    map.ExpectKeyGetValue(KNOWN_PEERS, item.know_peers_);
    map.ExpectKeyGetValue(KNOWN_URIS, item.known_uris_);
    map.ExpectKeyGetValue(CONNECTION_EXPIRY, item.connection_expiry_);
    map.ExpectKeyGetValue(DESIRED_EXPIRY, item.desired_uri_expiry_);
    map.ExpectKeyGetValue(DESIRED_PEERS, item.desired_peers_);
    map.ExpectKeyGetValue(DESIRED_URIS, item.desired_uris_);
  }
};
}  // namespace serializers
}  // namespace fetch