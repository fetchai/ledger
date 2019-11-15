#pragma once
#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/sha1.hpp"
#include "muddle/kademlia/bucket.hpp"
#include "muddle/kademlia/primitives.hpp"
#include "muddle/packet.hpp"

namespace fetch {
namespace muddle {

class KademliaTable
{
public:
  static constexpr uint64_t KADEMLIA_MAX_ID_BITS = KademliaAddress::ADDRESS_SIZE << 3;

  using Buckets     = std::array<Bucket, KADEMLIA_MAX_ID_BITS + 1>;
  using Peers       = std::deque<PeerInfo>;
  using RawAddress  = Packet::RawAddress;
  using PeerInfoPtr = std::shared_ptr<PeerInfo>;
  using PeerMap     = std::unordered_map<RawAddress, PeerInfoPtr>;
  using Port        = uint16_t;
  using PortList    = std::vector<Port>;

  KademliaTable(RawAddress const &own_address);

  /// Construtors & destructors
  /// @{
  KademliaTable(KademliaTable const &) = delete;
  KademliaTable(KademliaTable &&)      = delete;
  ~KademliaTable() override            = default;
  /// @}

  /// Operators
  /// @{
  KademliaTable &operator=(KademliaTable const &) = delete;
  KademliaTable &operator=(KademliaTable &&) = delete;
  /// @}

  /// Kademlia
  /// @{
  ConstByteArray Ping(RawAddress const &address, PortList port);
  Peers          FindPeer(RawAddress const &address);
  void           ReportLiveliness(RawAddress const &address);
  /// @}

  // TODO: add URI

  /// Trust interface
  /// @{
  // From Muddle
  void Blacklist(Address const &target);
  void Whitelist(Address const &target);
  bool IsBlacklisted(Address const &target) const;
  /// @}

  /// Peer selector interface
  /// @{
  void              AddDesiredPeer(Address const &address);
  void              AddDesiredPeer(Address const &address, network::Peer const &hint);
  void              RemoveDesiredPeer(Address const &address);
  Addresses         GetDesiredPeers() const;
  Addresses         GetKademliaPeers() const;  // TODO: Possibly delete
  Addresses         GetPendingRequests() const;
  PeersInfo         GetPeerCache() const;
  PeerSelectionMode GetMode() const;  // TODO: Possibly delete
  /// @}

  /// Periodic maintainance
  /// @{
  void Periodically() override;
  /// @}
private:
  mutable Mutex   mutex_;
  KademliaAddress own_address_;
  Buckets         buckets_;
  PeerMap         know_peers_;

  uint64_t kademlia_max_peers_per_bucket_{20};
};

}  // namespace muddle
}  // namespace fetch