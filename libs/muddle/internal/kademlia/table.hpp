#pragma once
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
  static constexpr uint64_t KADEMLIA_MAX_ID_BITS = KademliaAddress::ADDRESS_SIZE << 3;

  using Buckets        = std::array<Bucket, KADEMLIA_MAX_ID_BITS + 1>;
  using Peers          = std::deque<PeerInfo>;
  using Address        = Packet::Address;
  using PeerInfoPtr    = std::shared_ptr<PeerInfo>;
  using PeerMap        = std::unordered_map<Address, PeerInfoPtr>;
  using Port           = uint16_t;
  using PortList       = std::vector<Port>;
  using ConstByteArray = byte_array::ConstByteArray;

  KademliaTable(Address const &own_address);

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
  Peers          FindPeer(Address const &address, uint64_t start_bucket, bool scan_left = true,
                          bool scan_right = true);
  Peers          FindPeerByHamming(Address const &address);
  Peers FindPeerByHamming(Address const &address, uint64_t start_bucket, bool scan_left = true,
                          bool scan_right = true);
  void ReportLiveliness(Address const &address, Address const &reporter, PeerInfo const &info = {});
  void ReportExistence(PeerInfo const &info, Address const &reporter);
  void ReportFailure(Address const &address, Address const &reporter);
  PeerInfoPtr GetPeerDetails(Address const &address);
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

  std::string GetUri(Address const &address)
  {
    auto it = know_peers_.find(address);
    if (it == know_peers_.end())
    {
      return "";
    }
    return it->second->uri;
  }

  std::size_t active_buckets() const
  {
    std::size_t ret{0};
    for (auto &b : by_logarithm_)
    {
      ret += b.peers.size() > 0;
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

  uint64_t first_non_empty_bucket_{160};  // TODO: Test what happens in the case where
  uint64_t kademlia_max_peers_per_bucket_{20};
};

}  // namespace muddle
}  // namespace fetch