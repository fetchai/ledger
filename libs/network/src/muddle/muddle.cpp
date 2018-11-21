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

#include "network/muddle/muddle.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "network/muddle/muddle_register.hpp"
#include "network/muddle/muddle_server.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/tcp/tcp_server.hpp"

#include <chrono>
#include <thread>

using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace muddle {

static ConstByteArray ConvertAddress(Packet::RawAddress const &address)
{
  ByteArray output(address.size());
  std::copy(address.begin(), address.end(), output.pointer());

  return ConstByteArray{output};
}

static const auto        CLEANUP_INTERVAL        = std::chrono::seconds{10};
static std::size_t const MAINTENANCE_INTERVAL_MS = 2500;

/**
 * Constructs the muddle node instances
 *
 * @param certificate The certificate/identity of this node
 */
Muddle::Muddle(NetworkId network_id, Muddle::CertificatePtr &&certificate, NetworkManager const &nm)
  : certificate_(std::move(certificate))
  , identity_(certificate_->identity())
  , network_manager_(nm)
  , dispatcher_()
  , register_(std::make_shared<MuddleRegister>(dispatcher_))
  , router_(network_id, identity_.identifier(), *register_, dispatcher_)
  , thread_pool_(network::MakeThreadPool(1))
  , clients_(router_)
{
  char tmp[5];

  tmp[4] = 0;
  tmp[3] = char(network_id & 0xFF);
  network_id >>= 8;
  tmp[2] = char(network_id & 0xFF);
  network_id >>= 8;
  tmp[1] = char(network_id & 0xFF);
  network_id >>= 8;
  tmp[0] = char(network_id & 0xFF);

  network_id_str_ = std::string(tmp);
}

/**
 * Starts the muddle node and attaches it to the network
 *
 * @param initial_peer_list
 */
void Muddle::Start(PortList const &ports, UriList const &initial_peer_list)
{
  // start the thread pool
  thread_pool_->Start();
  router_.Start();

  FETCH_LOG_WARN(LOGGING_NAME, "MUDDLE START ", NetworkIdStr());

  // create all the muddle servers
  for (uint16_t port : ports)
  {
    CreateTcpServer(port);
  }

  // make the initial connections to the remote hosts
  for (auto const &peer : initial_peer_list)
  {
    // mark this peer as a persistent one
    clients_.AddPersistentPeer(peer);
  }

  // schedule the maintenance (which shall force the connection of the peers)
  RunPeriodicMaintenance();
}

const std::string &Muddle::NetworkIdStr()
{
  return network_id_str_;
}

/**
 * Stops the muddle node and removes it from the network
 */
void Muddle::Stop()
{
  thread_pool_->Stop();
  router_.Stop();

  // tear down all the servers
  servers_.clear();

  // tear down all the clients
  // TODO(EJF): Need to have a nice shutdown method
  // clients_.clear();
}

bool Muddle::GetOutgoingConnectionAddress(const Uri &uri, Address &address) const
{
  PeerConnectionList::Handle handle;
  if (!clients_.UriToHandle(uri, handle))
  {
    return false;
  }
  return router_.HandleToAddress(handle, address);
}

Muddle::ConnectionMap Muddle::GetConnections()
{
  ConnectionMap connection_map;

  auto const routing_table = router_.GetRoutingTable();
  auto const uri_map       = clients_.GetUriMap();

  for (auto const &entry : routing_table)
  {
    // convert the address to a byte array
    ConstByteArray address = ConvertAddress(entry.first);

    // based on the handle lookup the uri
    auto it = uri_map.find(entry.second.handle);
    if (it != uri_map.end())
    {
      // define the identity with a tcp:// or similar URI
      connection_map[address] = it->second;
    }
    else
    {
      // in the case that we do not have one then define it with a muddle URI
      connection_map[address] = Uri{"muddle://" + ToBase64(address)};
    }
  }

  return connection_map;
}

void Muddle::DropPeer(Address const &peer)
{
  router_.DropPeer(peer);
}

/**
 * Called periodically internally in order to co-ordinate network connections and clean up
 */
void Muddle::RunPeriodicMaintenance()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Running periodic maintenance");

  // connect to all the required peers
  for (Uri const &peer : clients_.GetPeersToConnectTo())
  {
    switch (peer.scheme())
    {
    case Uri::Scheme::Tcp:
      CreateTcpClient(peer);
      break;
    default:
      FETCH_LOG_ERROR(LOGGING_NAME, "Unable to create client connection to ", peer.uri());
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

  // schedule ourselves again a short time in the future
  thread_pool_->Post([this]() { RunPeriodicMaintenance(); }, MAINTENANCE_INTERVAL_MS);
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

  FETCH_LOG_DEBUG(LOGGING_NAME, "Start about to start server on port: ", port);

  // start it listening
  server->Start();

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
    FETCH_LOG_DEBUG(LOGGING_NAME, "Connection failed...");
    clients_.RemoveConnection(peer);
  });

  strong_conn->OnLeave([this, peer]() {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Connection left...to go where?");
    clients_.Disconnect(peer);
  });

  strong_conn->OnMessage([this, conn_handle](network::message_type const &msg) {
    try
    {
      // un-marshall the data
      serializers::ByteArrayBuffer buffer(msg);

      auto packet = std::make_shared<Packet>();
      buffer >> *packet;

      // dispatch the message to router
      router_.Route(conn_handle, packet);
    }
    catch (std::exception &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Error processing packet from ", conn_handle,
                      " error: ", ex.what());
    }
  });

  auto const &tcp_peer = peer.AsPeer();

  client.Connect(tcp_peer.address(), tcp_peer.port());
}

void Muddle::Blacklist(Address const &target)
{
  FETCH_LOG_WARN(LOGGING_NAME, "KLL:Blacklist", ToBase64(target));
  DropPeer(target);
  if (trust_system_!=nullptr && trust_system_->GetTrustUncertaintyOfPeer(target)>10.)
  {
    return;
  }
  router_.Blacklist(target);
}

void Muddle::Whitelist(Address const &target)
{
  router_.Whitelist(target);
}

}  // namespace muddle
}  // namespace fetch
