#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/macros.hpp"
#include "core/mutex.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "network/details/thread_pool.hpp"
#include "network/management/connection_register.hpp"
#include "network/muddle/dispatcher.hpp"
#include "network/muddle/peer_list.hpp"
#include "network/muddle/router.hpp"
#include "network/service/promise.hpp"
#include "network/tcp/abstract_server.hpp"
#include "network/uri.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>

namespace fetch {

namespace network {
class NetworkManager;
class AbstractConnection;
}  // namespace network

namespace muddle {

class MuddleRegister;
class MuddleEndpoint;

/**
 * The Top Level object for the muddle networking stack.
 *
 * Nodes connected into a Muddle are identified with a public key. With this abstraction when
 * interacting with this component network peers are identified with this public key instead of
 * ip address and port pairs.
 *
 * From a top level this stack is a combination of components that drives the P2P networking layer.
 * Fundamentally it is a collection of network connections which are attached to a router. When a
 * client wants to send a message it is done through the MuddleEndpoint interface. This ultimately
 * packages messages which are dispatched through the router.
 *
 * The router will determine the appropriate connection for the packet to be sent across. Similarly
 * when receiving packets. The router will either dispatch the message to one of the registered
 * clients (in the case when the message is addressed to the current node) or will endevour to
 * send the packet to the desired node. This is illustracted in the diagram below:
 *
 *                ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐
 *                                     Clients
 *                │                                               │
 *                 ─ ─ ─ ─ ─ ─ ─ ─ ─│─ ─ ─ ─ ─ ─│─ ─ ─ ─ ─ ─ ─ ─ ─
 *
 *                                  │           │
 *                ┌───────────────────────────────────────────────┐
 *                │                 │  Muddle   │                 │
 *                └───────────────────────────────────────────────┘
 *                                  │           │
 *
 *                                  │           │
 *                                  ▼           ▼
 *                                ┌───────────────┐
 *                                │               │
 *                                │               │
 *                                │    Router     │
 *                                │               │
 *                                │               │
 *                                └───────────────┘
 *                                   ▲    ▲    ▲
 *                                   │ ▲  │  ▲ │
 *                        ┌──────────┘ │  │  │ └──────────┐
 *                        │       ┌────┘  │  └────┐       │
 *                        │       │       │       │       │
 *                        ▼       ▼       ▼       ▼       ▼
 *                     ┌────┐  ┌────┐  ┌────┐  ┌────┐  ┌────┐
 *                     │    │  │    │  │    │  │    │  │    │
 *                     ├────┤  ├────┤  ├────┤  ├────┤  ├────┤
 *                     │    │  │    │  │    │  │    │  │    │
 *                     ├────┤  ├────┤  ├────┤  ├────┤  ├────┤
 *                     │    │  │    │  │    │  │    │  │    │
 *                     ├────┤  ├────┤  ├────┤  ├────┤  ├────┤
 *                     │    │  │    │  │    │  │    │  │    │
 *                     └────┘  └────┘  └────┘  └────┘  └────┘
 *
 *                         Underlying Network Connections
 *
 */
class Muddle
{
public:
  using CertificatePtr  = std::unique_ptr<crypto::Prover>;
  using Uri             = network::Uri;
  using UriList         = std::vector<Uri>;
  using NetworkManager  = network::NetworkManager;
  using Promise         = service::Promise;
  using PortList        = std::vector<uint16_t>;
  using Identity        = crypto::Identity;
  using Address         = Router::Address;
  using ConnectionState = PeerConnectionList::ConnectionState;
  using NetworkId       = MuddleEndpoint::NetworkId;
  using TrustSystem     = p2p::P2PTrustInterface<Muddle::Address>;

  using Handle = network::AbstractConnection::connection_handle_type;

  struct ConnectionData
  {
    Address         address;
    Uri             uri;
    ConnectionState state;
  };

  using ConnectionDataList = std::vector<ConnectionData>;
  using ConnectionMap      = std::unordered_map<Address, Uri>;

  static constexpr char const *LOGGING_NAME = "Muddle";

  /* Utility instance creation function. In a real application, create
     the muddle using loaded certificates and keys. In tests, call
     this to just get one now.*/

  static std::shared_ptr<Muddle> CreateMuddle(NetworkId                      network_id,
                                              fetch::network::NetworkManager tm)
  {
    crypto::ECDSASigner *certificate = new crypto::ECDSASigner();
    certificate->GenerateKeys();

    std::unique_ptr<crypto::Prover> certificate_;
    certificate_.reset(certificate);

    return std::make_shared<Muddle>(network_id, std::move(certificate_), tm);
  }

  static std::shared_ptr<Muddle> CreateMuddle(NetworkId                       network_id,
                                              std::unique_ptr<crypto::Prover> prover,
                                              fetch::network::NetworkManager  tm)
  {
    return std::make_shared<Muddle>(network_id, std::move(prover), tm);
  }

  static std::shared_ptr<Muddle> CreateMuddle(NetworkId                       network_id,
                                              std::unique_ptr<crypto::Prover> prover,
                                              fetch::network::NetworkManager  tm,
                                              TrustSystem *                   trust_system)
  {
    auto m = std::make_shared<Muddle>(network_id, std::move(prover), tm);
    m->SetUpTrust(trust_system);
    return m;
  }

  static inline uint32_t CreateNetworkId(const char *p)
  {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) |
           (uint32_t(p[3]));
  }

  // Construction / Destruction
  Muddle(NetworkId network_id, CertificatePtr &&certificate, NetworkManager const &nm);
  Muddle(Muddle const &) = delete;
  Muddle(Muddle &&)      = delete;
  ~Muddle()              = default;

  /// @name Top Level Node Control
  /// @{
  void Start(PortList const &ports, UriList const &initial_peer_list = UriList{});
  void Stop();
  /// @}

  Identity const &identity() const;

  MuddleEndpoint &AsEndpoint();

  ConnectionMap GetConnections();

  bool GetOutgoingConnectionAddress(const Uri &uri, Address &address) const;

  PeerConnectionList &useClients();

  /// @name Peer control
  /// @{
  void            AddPeer(Uri const &peer);
  void            DropPeer(Uri const &peer);
  void            DropPeer(Address const &peer);
  std::size_t     NumPeers() const;
  ConnectionState GetPeerState(Uri const &uri);

  void Blacklist(Address const &target);
  void Whitelist(Address const &target);
  bool IsBlacklisted(Address const &target) const;
  /// @}

  // Operators
  Muddle &operator=(Muddle const &) = delete;
  Muddle &operator=(Muddle &&) = delete;

  void Debug(std::string const &prefix)
  {
    router_.Debug(prefix);
    clients_.Debug(prefix);
  }

  const std::string &NetworkIdStr();

  bool HandleToIdentifier(const Handle &handle, byte_array::ConstByteArray &identifier) const
  {
    FETCH_LOG_WARN(LOGGING_NAME, "HandleToIdentifier: I am ",
                   byte_array::ToBase64(identity_.identifier()));
    return router_.HandleToAddress(handle, identifier);
  }

  void SetUpTrust(TrustSystem *trust_system)
  {
    trust_system_ = trust_system;
  }

private:
  using Server     = std::shared_ptr<network::AbstractNetworkServer>;
  using ServerList = std::vector<Server>;
  using Client     = std::shared_ptr<network::AbstractConnection>;
  using ThreadPool = network::ThreadPool;
  using Register   = std::shared_ptr<MuddleRegister>;
  using Mutex      = mutex::Mutex;
  using Lock       = std::lock_guard<Mutex>;
  using Clock      = std::chrono::system_clock;
  using Timepoint  = Clock::time_point;
  using Duration   = Clock::duration;

  void RunPeriodicMaintenance();

  void CreateTcpServer(uint16_t port);
  void CreateTcpClient(Uri const &peer);

  CertificatePtr const certificate_;      ///< The private and public keys for the node identity
  Identity const       identity_;         ///< Cached version of the identity (public key)
  NetworkManager       network_manager_;  ///< The network manager
  Dispatcher           dispatcher_;       ///< Waiting promise store
  Register             register_;         ///< The register for all the connection
  Router               router_;           ///< The packet router for the node
  ThreadPool           thread_pool_;      ///< The thread pool / task queue
  Mutex                servers_lock_{__LINE__, __FILE__};
  ServerList           servers_;  ///< The list of listening servers
  PeerConnectionList   clients_;  ///< The list of active and possible inactive connections
  Timepoint            last_cleanup_ = Clock::now();
  NetworkId            network_id_;
  std::string          network_id_str_;
  TrustSystem *        trust_system_ = nullptr;
};

inline Muddle::Identity const &Muddle::identity() const
{
  return identity_;
}

inline MuddleEndpoint &Muddle::AsEndpoint()
{
  return router_;
}

inline void Muddle::AddPeer(Uri const &peer)
{
  FETCH_LOG_WARN(LOGGING_NAME, "AddPeer: ", peer.ToString(), "    ", this);
  FETCH_LOG_WARN(LOGGING_NAME, "AddPeer: ", peer.ToString(), " to ", NetworkIdStr());
  FETCH_LOG_WARN(LOGGING_NAME, "AddPeer: ", peer.ToString(), "to  muddle ",
                 byte_array::ToBase64(identity_.identifier()));
  clients_.AddPersistentPeer(peer);
}

inline void Muddle::DropPeer(Uri const &peer)
{
  clients_.RemovePersistentPeer(peer);
}

inline std::size_t Muddle::NumPeers() const
{
  return clients_.GetNumPeers();
}

inline Muddle::ConnectionState Muddle::GetPeerState(Uri const &uri)
{
  return clients_.GetStateForPeer(uri);
}

}  // namespace muddle
}  // namespace fetch
