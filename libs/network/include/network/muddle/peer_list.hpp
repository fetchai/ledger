#ifndef FETCH_PEER_LIST_HPP
#define FETCH_PEER_LIST_HPP

#include "core/mutex.hpp"
#include "network/peer.hpp"
#include "network/management/abstract_connection.hpp"

#include <chrono>
#include <unordered_set>
#include <unordered_map>

namespace fetch {
namespace muddle {

class Router;

class PeerConnectionList
{
public:
  using Peer          = network::Peer;
  using PeerList      = std::vector<Peer>;
  using ConnectionPtr = std::shared_ptr<network::AbstractConnection>;
  using Handle        = network::AbstractConnection::connection_handle_type;

  static constexpr char const *LOGGING_NAME = "PeerConnList";

  // Construction / Destruction
  explicit PeerConnectionList(Router &router);
  PeerConnectionList(PeerConnectionList const &) = delete;
  PeerConnectionList(PeerConnectionList &&) = delete;
  ~PeerConnectionList() = default;

  // Operators
  PeerConnectionList &operator=(PeerConnectionList const &) = delete;
  PeerConnectionList &operator=(PeerConnectionList &&) = delete;

  /// @name Persistent connections
  /// @[
  void AddPersistentPeer(Peer const &peer);
  void RemovePersistentPeer(Peer const &peer);
  /// @}

  /// @name Peer based connection information
  /// @{
  void AddConnection(Peer const &peer, ConnectionPtr const &conn);
  void OnConnectionEstablished(Peer const &peer);
  void RemoveConnection(Peer const &peer);
  /// @}

  PeerList GetPeersToConnectTo() const;

private:

  using Clock = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;

  struct PeerMetadata
  {
    Timepoint   last_failed_connection;   ///< The last time a connection to a node failed
    std::size_t attempts = 0;
    std::size_t successes = 0;            ///< The total number of successful connections
    std::size_t consecutive_failures = 0;
    std::size_t total_failures = 0;       ///< The total number of connection failures
  };

  using Mutex        = mutex::Mutex;
  using Lock         = std::lock_guard<Mutex>;
  using PeerSet      = std::unordered_set<Peer>;
  using PeerMap      = std::unordered_map<Peer, ConnectionPtr>;
  using MetadataMap  = std::unordered_map<Peer, PeerMetadata>;
//  using ConnectionMap = std::unordered_map<Handle, ConnectionPtr>;

  Router        &router_;

  mutable Mutex lock_{__LINE__, __FILE__};
  PeerSet       persistent_peers_;
  PeerMap       peer_connections_;
  MetadataMap   peer_metadata_;
};

} // namespace p2p
} // namespace fetch

#endif //FETCH_PEER_LIST_HPP
