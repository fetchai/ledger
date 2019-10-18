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

#include "muddle.hpp"
#include "muddle_logging_name.hpp"
#include "muddle_register.hpp"
#include "muddle_registry.hpp"
#include "muddle_server.hpp"
#include "peer_selector.hpp"

#include "core/containers/set_intersection.hpp"
#include "core/logging.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/tcp/tcp_server.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <exception>
#include <sstream>

using namespace std::chrono_literals;

namespace fetch {
namespace muddle {

static auto const        CLEANUP_INTERVAL           = std::chrono::seconds{10};
static std::size_t const MAINTENANCE_INTERVAL_MS    = 2500;
static std::size_t const PEER_SELECTION_INTERVAL_MS = 500;

/**
 * Constructs the muddle node instances
 *
 * @param certificate The certificate/identity of this node
 */
Muddle::Muddle(NetworkId network_id, CertificatePtr certificate, NetworkManager const &nm,
               bool sign_packets, bool sign_broadcasts, std::string external_address)
  : name_{GenerateLoggingName("Muddle", network_id)}
  , certificate_(std::move(certificate))
  , external_address_(std::move(external_address))
  , node_address_(certificate_->identity().identifier())
  , network_manager_(nm)
  , dispatcher_(network_id, certificate_->identity().identifier())
  , register_(std::make_shared<MuddleRegister>(network_id))
  , router_(network_id, node_address_, *register_, dispatcher_,
            sign_packets ? certificate_.get() : nullptr, sign_packets && sign_broadcasts)
  , clients_(network_id)
  , network_id_(network_id)
  , reactor_{"muddle"}
  , maintenance_periodic_(std::make_shared<core::PeriodicFunctor>(
        std::chrono::milliseconds{MAINTENANCE_INTERVAL_MS}, this, &Muddle::RunPeriodicMaintenance))
  , direct_message_service_(node_address_, router_, *register_, clients_)
  , peer_selector_(std::make_shared<PeerSelector>(
        network_id, std::chrono::milliseconds{PEER_SELECTION_INTERVAL_MS}, reactor_, *register_,
        clients_, router_))
  , rpc_server_(router_, SERVICE_MUDDLE, CHANNEL_RPC)
{
  // handle the left issues
  register_->OnConnectionLeft([this](Handle handle) {
    router_.ConnectionDropped(handle);
    direct_message_service_.SignalConnectionLeft(handle);
  });

  // register the status update
  clients_.SetStatusCallback(
      [this](Uri const &peer, Handle handle, PeerConnectionList::ConnectionState state) {
        FETCH_UNUSED(peer);

        if (state == PeerConnectionList::ConnectionState::CONNECTED)
        {
          direct_message_service_.InitiateConnection(handle);
        }
      });

  rpc_server_.Add(RPC_MUDDLE_DISCOVERY, &discovery_service_);

  reactor_.Attach(maintenance_periodic_);
  reactor_.Attach(peer_selector_);
}

Muddle::~Muddle()
{
  MuddleRegistry::Instance().Unregister(this);

  // ensure the instance has stopped
  Stop();
}

/**
 * Start the muddle instance connecting to the initial set of peers and listing on the specified
 * set of ports
 *
 * @param peers The initial set of peers that muddle should connect to
 * @param ports The set of ports to listen on. Zero signals a random port
 * @return true if successful, otherwise false
 */
bool Muddle::Start(Peers const &peers, Ports const &ports)
{
  Uris uris{};
  for (auto const &peer : peers)
  {
    Uri uri{};
    if (!uri.Parse(peer))
    {
      return false;
    }

    // add the uri to the set
    uris.emplace(std::move(uri));
  }

  return Start(uris, ports);
}

/**
 * Start the muddle instance connecting to the initial set of peers and listing on the specified
 * set of ports
 *
 * @param peers The initial set of peers that muddle should connect to
 * @param ports The set of ports to listen on. Zero signals a random port
 * @return true if successful, otherwise false
 */
bool Muddle::Start(Uris const &peers, Ports const &ports)
{
  router_.Start();

  // make the initial connections to the remote hosts
  for (auto const &peer : peers)
  {
    // mark this peer as a persistent one
    clients_.AddPersistentPeer(peer);
  }

  // create all the muddle servers
  for (uint16_t port : ports)
  {
    CreateTcpServer(port);
  }

  // schedule the maintenance (which shall force the connection of the peers)
  RunPeriodicMaintenance();

  reactor_.Start();

  // register this muddle instance
  MuddleRegistry::Instance().Register(shared_from_this());

  // allow the muddle to start up
  std::this_thread::sleep_for(1s);

  return true;
}

/**
 * Start the muddle instance listing on the specified set of ports
 *
 * @param ports The set of ports to listen on. Zero signals a random port
 * @return true if successful, otherwise false
 */
bool Muddle::Start(Ports const &ports)
{
  return Start(Uris{}, ports);
}

/**
 * Stop the muddle instance, this will cause all the connected to close
 */
void Muddle::Stop()
{
  // stop all the periodic actions
  reactor_.Stop();
  router_.Stop();

  // tear down all the servers
  {
    FETCH_LOCK(servers_lock_);
    servers_.clear();
  }

  // client shutdown loop
  clients_.DisconnectAll();

  network_manager_.Stop();
}

/**
 * Get the endpoint interface for this muddle instance/
 *
 * @return The endpoint pointer
 */
MuddleEndpoint &Muddle::GetEndpoint()
{
  return router_;
}

/**
 * Get the associated network for this muddle instance
 *
 * @return The current network
 */
NetworkId const &Muddle::GetNetwork() const
{
  return network_id_;
}

/**
 * Get the address of this muddle node
 *
 * @return The address of the node
 */
Address const &Muddle::GetAddress() const
{
  return node_address_;
}

/**
 * Get the set of ports that the server is currently listening on
 * @return The set of server ports
 */
Muddle::Ports Muddle::GetListeningPorts() const
{
  Muddle::Ports ports{};

  FETCH_LOCK(servers_lock_);
  ports.reserve(servers_.size());
  for (auto const &server : servers_)
  {
    ports.emplace_back(server->GetListeningPort());
  }

  return ports;
}

/**
 * Get the set of addresses to whom this node is directly connected to
 *
 * @return The set of addresses
 */
Muddle::Addresses Muddle::GetDirectlyConnectedPeers() const
{
  return router_.GetDirectlyConnectedPeerSet();
}

/**
 * Get the set of addresses of peers that are connected directly to this node
 *
 * @return The set of addresses
 */
Muddle::Addresses Muddle::GetIncomingConnectedPeers() const
{
  return GetDirectlyConnectedPeers() & register_->GetIncomingAddressSet();
}

/**
 * Get the set of addresses of peers that we are directly connected to
 *
 * @return The set of peers
 */
Muddle::Addresses Muddle::GetOutgoingConnectedPeers() const
{
  return GetDirectlyConnectedPeers() & register_->GetOutgoingAddressSet();
}

/**
 * Get the number of peers that are directly connected to this node
 *
 * @return The number of directly connected peers
 */
std::size_t Muddle::GetNumDirectlyConnectedPeers() const
{
  auto const current_direct_peers = GetDirectlyConnectedPeers();
  return current_direct_peers.size();
}

/**
 * Determine if we are directly connected to the specified address
 *
 * @param address The address to check
 * @return true if directly connected, otherwise false
 */
bool Muddle::IsDirectlyConnected(Address const &address) const
{
  auto const current_direct_peers = GetDirectlyConnectedPeers();
  return current_direct_peers.find(address) != current_direct_peers.end();
}

/**
 * Query the current peer selection mode for this muddle
 *
 * @return The current mode
 */
PeerSelectionMode Muddle::GetPeerSelectionMode() const
{
  return peer_selector_->GetMode();
}

/**
 * Update the current peer selection mode for this muddle
 * @param mode
 */
void Muddle::SetPeerSelectionMode(PeerSelectionMode mode)
{
  router_.SetKademliaRouting(PeerSelectionMode::KADEMLIA == mode);
  peer_selector_->SetMode(mode);
}

/**
 * Get the set of addresses that have been requested to connect to
 *
 * @return The set of addresses
 */
Muddle::Addresses Muddle::GetRequestedPeers() const
{
  return peer_selector_->GetDesiredPeers();
}

/**
 * Request that muddle attempts to connect to the specified address
 *
 * @param address The requested address to connect to
 */
void Muddle::ConnectTo(Address const &address)
{
  if (node_address_ != address)
  {
    peer_selector_->AddDesiredPeer(address);
  }
}

/**
 * Request that muddle attempts to connect to the specified set of addresses
 *
 * @param addresses The set of addresses
 */
void Muddle::ConnectTo(Addresses const &addresses)
{
  for (auto const &address : addresses)
  {
    ConnectTo(address);
  }
}

void Muddle::ConnectTo(Address const &address, network::Uri const &uri_hint)
{
  if (node_address_ != address)
  {
    if (uri_hint.IsTcpPeer())
    {
      peer_selector_->AddDesiredPeer(address, uri_hint.GetTcpPeer());
    }
    else
    {
      FETCH_LOG_WARN(logging_name_, "Incompatible hint uri type: ", uri_hint.ToString());
    }
  }
}

void Muddle::ConnectTo(AddressHints const &address_hints)
{
  for (auto const &element : address_hints)
  {
    ConnectTo(element.first, element.second);
  }
}

/**
 * Request that muddle disconnected from the specified address
 *
 * @param address The requested address to disconnect from
 */
void Muddle::DisconnectFrom(Address const &address)
{
  peer_selector_->RemoveDesiredPeer(address);
}

/**
 * Request that muddle disconnects from the specified set of addresses
 *
 * @param addresses The set of addresses
 */
void Muddle::DisconnectFrom(Addresses const &addresses)
{
  for (auto const &address : addresses)
  {
    DisconnectFrom(address);
  }
}

/**
 * Update the confidence for a specified address to specified level
 *
 * @param address The address to be updated
 * @param confidence The confidence level to be set
 */
void Muddle::SetConfidence(Address const &address, Confidence confidence)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(confidence);
}

/**
 * Update the confidence for all the specified addresses with the specified level
 *
 * @param addresses The set of addresses to update
 * @param confidence The confidence level to be used
 */
void Muddle::SetConfidence(Addresses const &addresses, Confidence confidence)
{
  for (auto const &address : addresses)
  {
    SetConfidence(address, confidence);
  }
}

/**
 * Update a map of address to confidence level
 *
 * @param map The map of address to confidence level
 */
void Muddle::SetConfidence(ConfidenceMap const &map)
{
  for (auto const &element : map)
  {
    SetConfidence(element.first, element.second);
  }
}

Dispatcher const &Muddle::dispatcher() const
{
  return dispatcher_;
}

Router const &Muddle::router() const
{
  return router_;
}

MuddleRegister const &Muddle::connection_register() const
{
  return *register_;
}

PeerConnectionList const &Muddle::connection_list() const
{
  return clients_;
}

DirectMessageService const &Muddle::direct_message_service() const
{
  return direct_message_service_;
}

PeerSelector const &Muddle::peer_selector() const
{
  return *peer_selector_;
}

Muddle::ServerList const &Muddle::servers() const
{
  return servers_;
}

/**
 * Called periodically internally in order to co-ordinate network connections and clean up
 */
void Muddle::RunPeriodicMaintenance()
{
  FETCH_LOG_TRACE(logging_name_, "Running periodic maintenance");

  try
  {
    // update discovery information
    DiscoveryService::Peers external_addresses{};
    for (uint16_t port : GetListeningPorts())
    {
      // ignore pending ports
      if (port == 0)
      {
        continue;
      }

      external_addresses.emplace_back(network::Peer(external_address_, port));
      FETCH_LOG_TRACE(logging_name_, "Discovery: ", external_addresses.back().ToString());
    }
    discovery_service_.UpdatePeers(std::move(external_addresses));

    // connect to all the required peers
    for (Uri const &peer : clients_.GetPeersToConnectTo())
    {
      switch (peer.scheme())
      {
      case Uri::Scheme::Tcp:
        CreateTcpClient(peer);
        break;
      default:
        FETCH_LOG_ERROR(logging_name_, "Unable to create client connection to ", peer.uri());
        break;
      }
    }

    // run periodic cleanup
    Duration const time_since_last_cleanup = Clock::now() - last_cleanup_;
    if (time_since_last_cleanup >= CLEANUP_INTERVAL)
    {
      // clean up and pending message handlers and also trigger the timeout logic
      dispatcher_.Cleanup();

      // clean up echo caches and other temporary stored objects
      router_.Cleanup();

      last_cleanup_ = Clock::now();
    }
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_WARN(logging_name_, "Exception in periodic maintenance: ", e.what());
  }
}

/**
 * Creates a new TCP server to accept connections from specified port
 *
 * @param port The specified port to listen on
 * @return The newly created server
 */
void Muddle::CreateTcpServer(uint16_t port)
{
  using ServerImpl = MuddleServer<network::TCPServer>;

  // create the server
  auto server = std::make_shared<ServerImpl>(router_, port, network_manager_);

  // mark the server as managed by the register
  server->SetConnectionRegister(
      std::static_pointer_cast<network::AbstractConnectionRegister>(register_));

  // start it listening
  server->Start();

  FETCH_LOCK(servers_lock_);
  servers_.emplace_back(std::static_pointer_cast<network::AbstractNetworkServer>(server));
}

/**
 * Create a new TCP client connection to the specified peer
 *
 * @param peer The peer to connect to
 */
void Muddle::CreateTcpClient(Uri const &peer)
{
  using ClientImpl       = network::TCPClient;
  using ConnectionRegPtr = std::shared_ptr<network::AbstractConnectionRegister>;

  ClientImpl client(network_manager_);
  auto       conn = client.connection_pointer();

  auto strong_conn = conn.lock();
  assert(strong_conn);
  auto conn_handle = strong_conn->handle();

  FETCH_LOG_INFO(logging_name_, "Creating connection to ", peer.ToString(), " (conn: ", conn_handle,
                 ")");

  ConnectionRegPtr reg = std::static_pointer_cast<network::AbstractConnectionRegister>(register_);

  // register the connection with the register
  strong_conn->SetConnectionManager(reg);

  // manually trigger the connection enter phase
  reg->Enter(conn);

  // also add the connection to the client list
  clients_.AddConnection(peer, strong_conn);

  // debug handlers
  strong_conn->OnConnectionSuccess([this, peer]() { clients_.OnConnectionEstablished(peer); });

  strong_conn->OnConnectionFailed([this, peer]() {
    FETCH_LOG_INFO(logging_name_, "Connection to ", peer.ToString(), " failed");
    clients_.RemoveConnection(peer);
  });

  strong_conn->OnLeave([this, peer]() {
    FETCH_LOG_INFO(logging_name_, "Connection to ", peer.ToString(), " left");
    clients_.RemoveConnection(peer);
  });

  strong_conn->OnMessage([this, peer, conn_handle](network::MessageType const &msg) {
    try
    {
      auto packet = std::make_shared<Packet>();

      if (Packet::FromBuffer(*packet, msg.pointer(), msg.size()))
      {
        // dispatch the message to router
        router_.Route(conn_handle, packet);
      }
      else
      {
        FETCH_LOG_WARN(logging_name_, "Failed to read packet from buffer");
      }
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_ERROR(logging_name_, "Error processing packet from ", peer.ToString(),
                      " error: ", ex.what());
    }
  });

  auto const &tcp_peer = peer.GetTcpPeer();

  client.Connect(tcp_peer.address(), tcp_peer.port());
}

}  // namespace muddle
}  // namespace fetch
