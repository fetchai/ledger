#include "core/logger.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/tcp/tcp_server.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/muddle/muddle_register.hpp"
#include "network/muddle/muddle_server.hpp"
#include "network/muddle/muddle.hpp"

#include <thread>
#include <chrono>

namespace fetch {
namespace muddle {

static const auto CONNECTION_TIMEOUT = std::chrono::seconds{5};
static std::size_t const MAINTENANCE_INTERVAL_MS = 2500;

static char const *ConnTypeToString(uint16_t type) noexcept
{
  char const *name = "unknown";
  switch (type)
  {
    case network::AbstractConnection::TYPE_INCOMING:
      name = "incoming";
      break;
    case network::AbstractConnection::TYPE_OUTGOING:
      name = "outgoing";
      break;
    case network::AbstractConnection::TYPE_UNDEFINED:
      name = "undefined";
      break;
    default:
      break;
  }

  return name;
}

/**
 * Constructs the mudlle node instances
 *
 * @param certificate The certificate/identity of this node
 */
Muddle::Muddle(Muddle::CertificatePtr &&certificate, NetworkManager const &nm)
  : certificate_(std::move(certificate))
  , identity_(certificate_->identity())
  , network_manager_(nm)
  , dispatcher_()
  , register_(std::make_shared<MuddleRegister>(dispatcher_))
  , router_(identity_.identifier(), *register_, dispatcher_)
  , thread_pool_(network::MakeThreadPool(1))
  , clients_(router_)
{
}

/**
 * Starts the muddle node and attaches it to the network
 *
 * @param initial_peer_list
 */
void Muddle::Start(PortList const &ports, Muddle::PeerList const &initial_peer_list)
{
  // start the thread pool
  thread_pool_->Start();

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

  // start the first round of maintenance
  RunPeriodicMaintenance();
}

/**
 * Stops the muddle node and removes it from the network
 */
void Muddle::Stop()
{
  thread_pool_->Stop();

  // tear down all the servers
  servers_.clear();

  // tear down all the clients
  // TODO(EJF): Need to have a nice shutdown method
  //clients_.clear();
}

/**
 * Called periodically internally in order to co-ordinate network connections and clean up
 */
void Muddle::RunPeriodicMaintenance()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Running periodic maintenance");

  // connect to all the required peers
  for (auto const &peer : clients_.GetPeersToConnectTo())
  {
    CreateTcpClient(peer);
  }

  // debug
#if 1
  using ConnectionMap = MuddleRegister::ConnectionMap;

  // make a copy of the map to be processed for debug reasons
  ConnectionMap map;
  register_->VisitConnectionMap([&map](ConnectionMap const &m) {
    map = m;
  });

  FETCH_LOG_INFO(LOGGING_NAME, "Current Map Size: ", map.size());

#if 0
  for (auto const &element : map)
  {
    auto const &handle = element.first;
    auto const &conn = element.second;

    auto strong_conn = conn.lock();
    if (strong_conn)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "- Handle: ", handle, " type: ", ConnTypeToString(strong_conn->Type()));
    }
  }
#endif
#endif

  // schedule the main
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
    std::static_pointer_cast<network::AbstractConnectionRegister>(register_)
  );

  FETCH_LOG_DEBUG(LOGGING_NAME, "Start about to start server on port: ", port);

  // start it listening
  server->Start();

  servers_.emplace_back(std::static_pointer_cast<network::AbstractNetworkServer>(server));
}

void Muddle::CreateTcpClient(network::Peer const &peer)
{
  using ClientImpl = network::TCPClient;
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
  strong_conn->OnConnectionSuccess([this, peer]() {
    clients_.OnConnectionEstablished(peer);
  });

  strong_conn->OnConnectionFailed([this, peer]() {
    FETCH_LOG_INFO(LOGGING_NAME, "Connection failed...");
    clients_.RemoveConnection(peer);
  });

  strong_conn->OnLeave([this, peer]() {
    FETCH_LOG_INFO(LOGGING_NAME, "Connection left...to go where?");
    clients_.RemoveConnection(peer);
  });

  strong_conn->OnMessage([this, conn_handle](network::message_type const &msg) {
    FETCH_LOG_INFO(LOGGING_NAME, "Got Message");

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
      FETCH_LOG_ERROR(LOGGING_NAME, "Error processing packet from ", conn_handle, " error: ", ex.what());
    }
  });

  client.Connect(peer.address(), peer.port());

//#if 1
//  // wait for the connection to be established
//  thread_pool_->Post([strong_conn]() {
//    using Clock = std::chrono::high_resolution_clock;
//
//    // connection loop
//    auto const start = Clock::now();
//    for (;;)
//    {
//      if (strong_conn->is_alive())
//      {
//        break;
//      }
//
//      auto const delta = Clock::now() - start;
//      if (delta > CONNECTION_TIMEOUT)
//      {
//        FETCH_LOG_INFO(LOGGING_NAME, "Timed out waiting for socket to connect to remote host");
//        break;
//      }
//
//      std::this_thread::sleep_for(std::chrono::milliseconds{10});
//    }
//
//    // ensure
//  });
//#endif
}



} // namespace p2p
} // namespace fetch
