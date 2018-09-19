#ifndef FETCH_P2P_MUDDLE_HPP
#define FETCH_P2P_MUDDLE_HPP

#include "core/macros.hpp"
#include "core/mutex.hpp"
#include "crypto/prover.hpp"
#include "network/uri.hpp"
#include "network/tcp/abstract_server.hpp"
#include "network/service/promise.hpp"
#include "network/details/thread_pool.hpp"
#include "network/muddle/peer_list.hpp"
#include "network/muddle/dispatcher.hpp"
#include "network/muddle/router.hpp"
#include "network/management/connection_register.hpp"

#include <memory>
#include <chrono>
#include <vector>
#include <unordered_map>

namespace fetch {

namespace network {
  class NetworkManager;
  class AbstractConnection;
}

namespace muddle {

class MuddleRegister;
class MuddleEndpoint;

// TODO(EJF): Entanglement
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

  struct ConnectionData
  {
    Address address;
    Uri uri;
    ConnectionState state;
  };

  using ConnectionDataList = std::vector<ConnectionData>;
  using ConnectionMap = std::unordered_map<Address, Uri>;

  static constexpr char const *LOGGING_NAME = "Muddle";

  // Construction / Destruction
  Muddle(CertificatePtr &&certificate, NetworkManager const &nm);
  Muddle(Muddle const &) = delete;
  Muddle(Muddle &&) = delete;
  ~Muddle() = default;

  /// @name Top Level Node Control
  /// @{
  void Start(PortList const &ports, UriList const &initial_peer_list = UriList{});
  void Stop();
  /// @}

  Identity const &identity() const;

  MuddleEndpoint &AsEndpoint();

  ConnectionMap GetConnections();

  PeerConnectionList &useClients();

  /// @name Peer control
  /// @{
  void AddPeer(Uri const &peer);
  void DropPeer(Uri const &peer);
  std::size_t NumPeers() const;
  ConnectionState GetPeerState(Uri const &uri);
  /// @}

  // Operators
  Muddle &operator=(Muddle const &) = delete;
  Muddle &operator=(Muddle &&) = delete;

private:

  using Server      = std::shared_ptr<network::AbstractNetworkServer>;
  using ServerList  = std::vector<Server>;
  using Client      = std::shared_ptr<network::AbstractConnection>;
  using ThreadPool  = network::ThreadPool;
  using Register    = std::shared_ptr<MuddleRegister>;
  using Mutex       = mutex::Mutex;
  using Lock        = std::lock_guard<Mutex>;
  using Clock       = std::chrono::system_clock;
  using Timepoint   = Clock::time_point;
  using Duration    = Clock::duration;

  void RunPeriodicMaintenance();

  void CreateTcpServer(uint16_t port);
  void CreateTcpClient(Uri const &peer);

  CertificatePtr const  certificate_;                         ///< The private and public keys for the node identity
  Identity const        identity_;                            ///< Cached version of the identity (public key)
  NetworkManager        network_manager_;                     ///< The network manager
  Dispatcher            dispatcher_;                          ///< Object that maintains the list of waiting promises and message consumers
  Register              register_;                            ///< The register for all the connection
  Router                router_;                              ///< The packet router for the node
  ThreadPool            thread_pool_;                         ///< The thread pool / task queue
  Mutex                 servers_lock_{__LINE__, __FILE__};
  ServerList            servers_;                             ///< The list of listening servers
  PeerConnectionList    clients_;                             ///< The list of active and possible inactive connections
  Timepoint             last_cleanup_ = Clock::now();
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

} // namespace p2p
} // namespace fetch

#endif //FETCH_P2P_MUDDLE_HPP
