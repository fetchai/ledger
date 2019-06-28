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

#include "core/mutex.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/uri.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

  using UriMap = std::unordered_map<Handle, Uri>;

  static constexpr char const *LOGGING_NAME = "PeerConnList";

  // Construction / Destruction
  explicit PeerConnectionList(Router &router);
  PeerConnectionList(PeerConnectionList const &) = delete;
  PeerConnectionList(PeerConnectionList &&)      = delete;
  ~PeerConnectionList()                          = default;

  // Operators
  PeerConnectionList &operator=(PeerConnectionList const &) = delete;
  PeerConnectionList &operator=(PeerConnectionList &&) = delete;

  /// @name Persistent connections
  /// @{
  bool AddPersistentPeer(Uri const &peer);
  void RemovePersistentPeer(Uri const &peer);
  void RemovePersistentPeer(Handle handle);

  std::size_t GetNumPeers() const;
  /// @}

  /// @name Peer based connection information
  /// @{
  void AddConnection(Uri const &peer, ConnectionPtr const &conn);
  void OnConnectionEstablished(Uri const &peer);
  void RemoveConnection(Uri const &peer);
  void RemoveConnection(Handle handle);
  void Disconnect(Uri const &peer);
  /// @}

  ConnectionState GetStateForPeer(Uri const &peer) const;

  PeerList GetPeersToConnectTo() const;

  PeerMap GetCurrentPeers() const;

  UriMap GetUriMap() const;
  Handle UriToHandle(const Uri &uri) const;

  void Debug(std::string const &prefix) const;

private:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;

  struct PeerMetadata
  {
    Timepoint   last_failed_connection;  ///< The last time a connection to a node failed.
    std::size_t attempts             = 0;
    std::size_t successes            = 0;  ///< The total number of successful connections.
    std::size_t consecutive_failures = 0;
    std::size_t total_failures       = 0;      ///< The total number of connection failures.
    bool        connected            = false;  ///< Whether the last/current attempt has succeeded.
  };

  using Mutex   = mutex::Mutex;
  using Lock    = std::lock_guard<Mutex>;
  using PeerSet = std::unordered_set<Uri>;

  using MetadataMap = std::unordered_map<Uri, PeerMetadata>;

  Router &router_;

  mutable Mutex lock_{__LINE__, __FILE__};
  PeerSet       persistent_peers_;
  PeerMap       peer_connections_;
  MetadataMap   peer_metadata_;

  bool ReadyForRetry(const PeerMetadata &metadata) const;
};

}  // namespace muddle
}  // namespace fetch
