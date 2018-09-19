#ifndef FETCH_PEER_LIST_HPP
#define FETCH_PEER_LIST_HPP

#include "core/mutex.hpp"
#include "network/uri.hpp"
#include "network/management/abstract_connection.hpp"

#include <chrono>
#include <unordered_set>
#include <unordered_map>

namespace fetch {
namespace muddle {

class Router;

/**
 * The peer connection list manages (and owns) the outgoing muddle connections. In the event that
 * a connection failure occurs, the peer connection list will be notified and it will apply an
 * exponential backoff strategy to retrying connections.
 */
class PeerConnectionList
{
public:
  using Uri           = network::Uri;
  using PeerList      = std::vector<Uri>;
  using ConnectionPtr = std::shared_ptr<network::AbstractConnection>;
  using Handle        = network::AbstractConnection::connection_handle_type;
  using PeerMap       = std::unordered_map<Uri, ConnectionPtr>;

  enum class ConnectionState
  {
    UNKNOWN = 0,
    TRYING  = 0x20,

    CONNECTED = 0x100,

    REMOTE = 0x200,

    INCOMING = 0x300,

    BACKOFF   = 0x10,
    BACKOFF_2 = 0x11,
    BACKOFF_3 = 0x12,
    BACKOFF_4 = 0x13,
    BACKOFF_5 = 0x14,
  };

  using UriMap        = std::unordered_map<Handle, Uri>;

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
  /// @{
  void AddPersistentPeer(Uri const &peer);
  void RemovePersistentPeer(Uri const &peer);
  std::size_t GetNumPeers() const;
  /// @}

  /// @name Peer based connection information
  /// @{
  void AddConnection(Uri const &peer, ConnectionPtr const &conn);
  void OnConnectionEstablished(Uri const &peer);
  void RemoveConnection(Uri const &peer);
  /// @}


  ConnectionState GetStateForPeer(Uri const &peer);

  PeerList GetPeersToConnectTo() const;

  PeerMap GetCurrentPeers() const;

  UriMap GetUriMap() const;

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
    bool        connected = false;        ///< Whether the last/current attempt has succeeded.
  };

  using Mutex        = mutex::Mutex;
  using Lock         = std::lock_guard<Mutex>;
  using PeerSet      = std::unordered_set<Uri>;

  using MetadataMap  = std::unordered_map<Uri, PeerMetadata>;

  Router        &router_;

  mutable Mutex lock_{__LINE__, __FILE__};
  PeerSet       persistent_peers_;
  PeerMap       peer_connections_;
  MetadataMap   peer_metadata_;

  bool ReadyForRetry(const PeerMetadata &metadata) const;
};

} // namespace p2p
} // namespace fetch

#endif //FETCH_PEER_LIST_HPP
