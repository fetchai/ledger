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

#include "direct_message_service.hpp"
#include "discovery_service.hpp"
#include "peer_list.hpp"
#include "router.hpp"

#include "core/macros.hpp"
#include "core/mutex.hpp"
#include "core/periodic_functor.hpp"
#include "core/reactor.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/network_id.hpp"
#include "muddle/rpc/server.hpp"
#include "network/details/thread_pool.hpp"

#include "network/service/promise.hpp"
#include "network/tcp/abstract_server.hpp"
#include "network/uri.hpp"

#include <chrono>
#include <memory>
#include <string>
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
class PeerTracker;

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
 * clients (in the case when the message is addressed to the current node) or will endeavour to
 * send the packet to the desired node. This is illustrated in the diagram below:
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
class Muddle : public MuddleInterface, public std::enable_shared_from_this<Muddle>
{
public:
  using CertificatePtr  = std::shared_ptr<crypto::Prover>;
  using Uri             = network::Uri;
  using UriList         = std::vector<Uri>;
  using NetworkManager  = network::NetworkManager;
  using Promise         = service::Promise;
  using Identity        = crypto::Identity;
  using Address         = Router::Address;
  using ConnectionState = PeerConnectionList::ConnectionState;
  using Handle          = network::AbstractConnection::ConnectionHandleType;
  using Server          = std::shared_ptr<network::AbstractNetworkServer>;
  using ServerList      = std::vector<Server>;
  using PeerTrackerPtr  = std::shared_ptr<PeerTracker>;
  using Clock           = std::chrono::system_clock;
  using Timepoint       = Clock::time_point;
  using Duration        = Clock::duration;

  struct ConnectionData
  {
    Address         address;
    Uri             uri;
    ConnectionState state;
  };

  using ConnectionDataList = std::vector<ConnectionData>;
  using ConnectionMap      = std::unordered_map<Address, Uri>;

  // Construction / Destruction
  Muddle(NetworkId network_id, CertificatePtr certificate, NetworkManager const &nm,
         std::string external_address = "127.0.0.1");
  Muddle(Muddle const &) = delete;
  Muddle(Muddle &&)      = delete;
  ~Muddle() override;

  /// @name Muddle Setup
  /// @{
  void            SetPeerTableFile(std::string const &filename) override;
  bool            Start(Peers const &peers, Ports const &ports) override;
  bool            Start(Uris const &peers, Ports const &ports) override;
  bool            Start(Uris const &peers, PortMapping const &port_mapping) override;
  bool            Start(Ports const &ports) override;
  void            Stop() override;
  MuddleEndpoint &GetEndpoint() override;
  /// @}

  /// @name Muddle Status
  /// @{
  NetworkId const &  GetNetwork() const override;
  Address const &    GetAddress() const override;
  std::string const &GetExternalAddress() const override;
  Ports              GetListeningPorts() const override;
  Addresses          GetDirectlyConnectedPeers() const override;
  Addresses          GetIncomingConnectedPeers() const override;
  Addresses          GetOutgoingConnectedPeers() const override;

  std::size_t GetNumDirectlyConnectedPeers() const override;
  bool        IsDirectlyConnected(Address const &address) const override;
  bool        IsConnectingOrConnected(Address const &address) const override;
  /// @}

  /// @name Peer Control
  /// @{

  Addresses GetRequestedPeers() const override;

  void ConnectTo(Address const &address, Duration const &expire) override;
  void ConnectTo(Addresses const &addresses, Duration const &expire) override;
  void ConnectTo(network::Uri const &uri, Duration const &expire) override;
  void ConnectTo(Address const &address, network::Uri const &uri_hint,
                 Duration const &expire) override;
  void ConnectTo(AddressHints const &address_hints, Duration const &expire) override;
  void DisconnectFrom(Address const &address) override;
  void DisconnectFrom(Addresses const &addresses) override;
  void SetConfidence(Address const &address, Confidence confidence) override;
  void SetConfidence(Addresses const &addresses, Confidence confidence) override;
  void SetConfidence(ConfidenceMap const &map) override;
  void SetTrackerConfiguration(TrackerConfiguration const &config) override;
  /// @}

  /// @name Internal Accessors
  /// @{
  Router const &              router() const;
  MuddleRegister const &      connection_register() const;
  PeerConnectionList const &  connection_list() const;
  DirectMessageService const &direct_message_service() const;
  PeerTracker const &         peer_tracker() const;
  ServerList const &          servers() const;
  /// @}

  // Operators
  Muddle &operator=(Muddle const &) = delete;
  Muddle &operator=(Muddle &&) = delete;

private:
  using Client     = std::shared_ptr<network::AbstractConnection>;
  using ThreadPool = network::ThreadPool;
  using Register   = std::shared_ptr<MuddleRegister>;

  void RunPeriodicMaintenance();
  void UpdateExternalAddresses();
  void CreateTcpServer(uint16_t port);
  void CreateTcpClient(Uri const &peer);

  std::string const    name_;
  char const *const    logging_name_{name_.c_str()};
  CertificatePtr const certificate_;  ///< The private and public keys for the node identity
  std::string const    external_address_;
  Address const        node_address_;
  NetworkManager       network_manager_;  ///< The network manager
  Register             register_;         ///< The register for all the connection
  Router               router_;           ///< The packet router for the node
  PortMapping          port_mapping_;
  std::atomic<bool>    stopping_{false};

  mutable Mutex servers_lock_;
  ServerList    servers_;  ///< The list of listening servers

  PeerConnectionList clients_;  ///< The list of active and possible inactive connections
  Timepoint          last_cleanup_ = Clock::now();
  NetworkId          network_id_;

  // Reactor and periodics
  core::Reactor        reactor_;
  core::RunnablePtr    maintenance_periodic_;
  DirectMessageService direct_message_service_;
  PeerTrackerPtr       peer_tracker_;

  // Services
  rpc::Server      rpc_server_;
  DiscoveryService discovery_service_{};
};

}  // namespace muddle
}  // namespace fetch
