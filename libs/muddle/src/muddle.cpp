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

#include "muddle.hpp"
#include "muddle_logging_name.hpp"
#include "muddle_register.hpp"
#include "muddle_registry.hpp"
#include "muddle_server.hpp"

#include "core/containers/set_intersection.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "kademlia/peer_tracker.hpp"
#include "logging/logging.hpp"
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
static std::size_t const PEER_SELECTION_INTERVAL_MS = 2500;

/**
 * Constructs the muddle node instances
 *
 * @param certificate The certificate/identity of this node
 */
Muddle::Muddle(NetworkId network_id, CertificatePtr certificate, NetworkManager const &nm,
               std::string external_address)
  : name_{GenerateLoggingName("Muddle", network_id)}
  , certificate_(std::move(certificate))
  , external_address_(std::move(external_address))
  , node_address_(certificate_->identity().identifier())
  , network_manager_(nm)
  , register_(std::make_shared<MuddleRegister>(network_id))
  , router_(network_id, node_address_, *register_, *certificate_)
  , clients_(network_id)
  , network_id_(network_id)
  , reactor_{"muddle"}
  , maintenance_periodic_(std::make_shared<core::PeriodicFunctor>(
        "Muddle", std::chrono::milliseconds{MAINTENANCE_INTERVAL_MS}, this,
        &Muddle::RunPeriodicMaintenance))
  , direct_message_service_(node_address_, router_, *register_, clients_)
  , peer_tracker_(PeerTracker::New(std::chrono::milliseconds{PEER_SELECTION_INTERVAL_MS}, reactor_,
                                   *register_, clients_, router_))
  , rpc_server_(router_, SERVICE_MUDDLE, CHANNEL_RPC)
{
  // Default configuration is to do no tracking at all
  peer_tracker_->SetConfiguration(TrackerConfiguration::DefaultConfiguration());
  router_.SetTracker(peer_tracker_);

  // handle the left issues
  register_->OnConnectionLeft([this](Handle handle) {
    peer_tracker_->RemoveConnectionHandle(handle);
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
  reactor_.Attach(peer_tracker_);
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

void Muddle::SetPeerTableFile(std::string const &filename)
{
  peer_tracker_->SetCacheFile(filename);
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
  stopping_ = false;

  // Setting ports prior to starting as a fallback mechanism
  // for giving details of peer
  peer_tracker_->UpdateExternalPorts(ports);
  peer_tracker_->Start();

  // Starting the router
  router_.Start();

  // create all the muddle servers
  // note that we want to start the servers first and then
  // the clients, as incoming connections will be requested
  // for uris
  for (uint16_t port : ports)
  {
    CreateTcpServer(port);
  }

  // Updating external addresses to make this peer discoverable
  UpdateExternalAddresses();

  // make the initial connections to the remote hosts
  for (auto const &peer : peers)
  {
    // mark this peer as a persistent one
    clients_.AddPersistentPeer(peer);
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

bool Muddle::Start(Uris const &peers, PortMapping const &port_mapping)
{
  Ports ports{};

  for (auto const &item : port_mapping)
  {
    // mapping a random port does not make any sense so this is a failure
    if (item.first == 0)
    {
      return false;
    }

    ports.emplace_back(item.first);
  }

  port_mapping_ = port_mapping;

  return Start(peers, ports);
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
  stopping_ = true;
  peer_tracker_->Stop();

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
 * Get the external address of the muddle
 *
 * @return The external address of the node
 */
std::string const &Muddle::GetExternalAddress() const
{
  return external_address_;
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

bool Muddle::IsConnectingOrConnected(Address const &address) const
{
  auto const desired = peer_tracker_->GetDesiredPeers();
  return desired.find(address) != desired.end();
}

/**
 * Get the set of addresses that have been requested to connect to
 *
 * @return The set of addresses
 */
Muddle::Addresses Muddle::GetRequestedPeers() const
{
  return peer_tracker_->GetDesiredPeers();
}

/**
 * Request that muddle attempts to connect to the specified address
 *
 * @param address The requested address to connect to
 */
void Muddle::ConnectTo(Address const &address, Duration const &expire)
{
  if (node_address_ != address)
  {
    peer_tracker_->AddDesiredPeer(address, expire);
  }
}

/**
 * Request that muddle attempts to connect to the specified set of addresses
 *
 * @param addresses The set of addresses
 */
void Muddle::ConnectTo(Addresses const &addresses, Duration const &expire)
{
  for (auto const &address : addresses)
  {
    ConnectTo(address, expire);
  }
}

/**
 * Request the muddle to make a persistent connection to a URI.
 *
 * @param uri The uri to connect to
 */
void Muddle::ConnectTo(network::Uri const &uri, Duration const &expire)
{
  clients_.AddPersistentPeer(uri);
  peer_tracker_->AddDesiredPeer(uri, expire);
}

void Muddle::ConnectTo(Address const &address, network::Uri const &uri_hint, Duration const &expire)
{
  if (address.empty())
  {
    FETCH_LOG_WARN(logging_name_,
                   "Address is empty, use ConnectTo(uri) to connect directly to uri.",
                   uri_hint.ToString());
    ConnectTo(uri_hint, expire);
  }
  else if (node_address_ != address)
  {
    if (uri_hint.IsTcpPeer())
    {
      peer_tracker_->AddDesiredPeer(address, uri_hint.GetTcpPeer(), expire);
    }
    else
    {
      FETCH_LOG_WARN(logging_name_, "Incompatible hint uri type: ", uri_hint.ToString());
    }
  }
}

void Muddle::ConnectTo(AddressHints const &address_hints, Duration const &expire)
{
  for (auto const &element : address_hints)
  {
    ConnectTo(element.first, element.second, expire);
  }
}

/**
 * Request that muddle disconnected from the specified address
 *
 * @param address The requested address to disconnect from
 */
void Muddle::DisconnectFrom(Address const &address)
{
  peer_tracker_->RemoveDesiredPeer(address);
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
  // TODO(tfr): implementation missing.
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

void Muddle::SetTrackerConfiguration(TrackerConfiguration const &config)
{
  peer_tracker_->SetConfiguration(config);
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

PeerTracker const &Muddle::peer_tracker() const
{
  return *peer_tracker_;
}

Muddle::ServerList const &Muddle::servers() const
{
  return servers_;
}

void Muddle::UpdateExternalAddresses()
{
  PeerTracker::NetworkUris external_uris{};
  DiscoveryService::Peers  external_addresses{};
  for (uint16_t port : GetListeningPorts())
  {
    // ignore pending ports
    if (port == 0)
    {
      continue;
    }

    // determine if the port needs to be mapped to an external range
    auto const it = port_mapping_.find(port);
    if (it != port_mapping_.end())
    {
      port = it->second;
    }

    network::Peer peer{external_address_, port};

    Uri uri;
    uri.Parse(peer.ToUri());

    external_uris.emplace_back(uri);

    external_addresses.emplace_back(std::move(peer));
    FETCH_LOG_TRACE(logging_name_, "Discovery: ", external_addresses.back().ToString());
  }

  discovery_service_.UpdatePeers(external_addresses);
  peer_tracker_->UpdateExternalUris(external_uris);
}

/**
 * Called periodically internally in order to co-ordinate network connections and clean up
 */
void Muddle::RunPeriodicMaintenance()
{
  // If we are stopping the muddle, we do not want to connect to new nodes
  // and otherwise do periodic maintenance.
  if (stopping_)
  {
    return;
  }

  FETCH_LOG_TRACE(logging_name_, "Running periodic maintenance");
  try
  {
    UpdateExternalAddresses();

    // update discovery information
    std::unordered_set<Uri> just_connected_to;

    // connect to all the required peers
    for (Uri const &peer : clients_.GetPeersToConnectTo())
    {
      // skipping uris we just connected to
      if (just_connected_to.find(peer) != just_connected_to.end())
      {
        FETCH_LOG_WARN(logging_name_, "Already connected. Skipping ", peer.uri());
        continue;
      }

      // connecting according to scheme
      switch (peer.scheme())
      {
      case Uri::Scheme::Tcp:
        CreateTcpClient(peer);
        break;
      default:
        FETCH_LOG_ERROR(logging_name_, "Unable to create client connection to ", peer.uri());
        break;
      }

      just_connected_to.emplace(peer);
    }

    // run periodic cleanup
    Duration const time_since_last_cleanup = Clock::now() - last_cleanup_;
    if (time_since_last_cleanup >= CLEANUP_INTERVAL)
    {
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
  std::weak_ptr<PeerTracker> wptr = peer_tracker_;
  strong_conn->OnConnectionSuccess([this, peer, wptr]() {
    auto ptr = wptr.lock();
    if (ptr)
    {
      peer_tracker_->ReportSuccessfulConnectAttempt(peer);
    }
    clients_.OnConnectionEstablished(peer);
  });

  strong_conn->OnConnectionFailed([this, peer, wptr]() {
    auto ptr = wptr.lock();
    if (ptr)
    {
      ptr->ReportFailedConnectAttempt(peer);
    }

    clients_.RemoveConnection(peer);
  });

  strong_conn->OnLeave([this, peer, wptr]() {
    auto ptr = wptr.lock();
    if (ptr)
    {
      ptr->ReportLeaving(peer);
    }

    clients_.RemoveConnection(peer);
  });

  std::weak_ptr<Muddle> weak_self = shared_from_this();
  strong_conn->OnMessage([this, peer, conn_handle, weak_self](network::MessageBuffer const &msg) {
    auto ptr = weak_self.lock();
    if (ptr)
    {
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
    }
  });

  auto const &tcp_peer = peer.GetTcpPeer();

  client.Connect(tcp_peer.address(), tcp_peer.port());
}

}  // namespace muddle
}  // namespace fetch
