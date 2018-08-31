#ifndef FETCH_P2P_MUDDLE_HPP
#define FETCH_P2P_MUDDLE_HPP

#include "core/macros.hpp"
#include "core/mutex.hpp"
#include "crypto/prover.hpp"
#include "network/peer.hpp"
#include "network/tcp/abstract_server.hpp"
#include "network/service/promise.hpp"
#include "network/details/thread_pool.hpp"
#include "network/muddle/peer_list.hpp"
#include "network/muddle/dispatcher.hpp"
#include "network/muddle/router.hpp"
#include "network/management/connection_register.hpp"

#include <memory>

namespace fetch {

namespace network {
  class NetworkManager;
  class AbstractConnection;
}

namespace muddle {

  class MuddleRegister;
  class MuddleEndpoint;

  /**
   * The muddle is the top level component of the
   */

  struct ConnectionMetadata
  {

  };

  // TODO(EJF): Entanglement
  class Muddle
  {
  public:

    using CertificatePtr  = std::unique_ptr<crypto::Prover>;
    using PeerList        = std::vector<network::Peer>;
    using NetworkManager  = network::NetworkManager;
    using Promise         = service::Promise;
    using PortList        = std::vector<uint16_t>;

    static constexpr char const *LOGGING_NAME = "Muddle";

    // Construction / Destruction
    Muddle(CertificatePtr &&certificate, NetworkManager const &nm);
    Muddle(Muddle const &) = delete;
    Muddle(Muddle &&) = delete;
    ~Muddle() = default;

    // Operators
    Muddle &operator=(Muddle const &) = delete;
    Muddle &operator=(Muddle &&) = delete;

    /// @name Top Level Node Control
    /// @{
    void Start(PortList const &ports, PeerList const & initial_peer_list = PeerList{});
    void Stop();
    /// @}

    MuddleEndpoint &AsEndpoint() { return router_; }

  private:

    using Server      = std::shared_ptr<network::AbstractNetworkServer>;
    using ServerList  = std::vector<Server>;
    using Client      = std::shared_ptr<network::AbstractConnection>;
    using ClientList  = PeerConnectionList;
    using Identity    = crypto::Identity;
    using ThreadPool  = network::ThreadPool;
    using Register    = std::shared_ptr<MuddleRegister>;
    using Mutex       = mutex::Mutex;
    using Lock        = std::lock_guard<Mutex>;

    void RunPeriodicMaintenance();

    void CreateTcpServer(uint16_t port);
    void CreateTcpClient(network::Peer const &peer);

    CertificatePtr const  certificate_;                             ///< The private and public keys for the node identity
    Identity const        identity_;                                ///< Cached version of the identity (public key)
    NetworkManager        network_manager_;                         ///< The network manager
    Dispatcher            dispatcher_;                              ///< Object that maintains the list of waiting promises and message consumers
    Register              register_;                                ///< The register for all the connection
    Router                router_;                                  ///< The packet router for the node
    ThreadPool            thread_pool_;                             ///< The thread pool / task queue
    Mutex                 servers_lock_{__LINE__, __FILE__};
    ServerList            servers_; ///< The list of listening servers
    ClientList            clients_; ///< The list of active and possible inactive connections
  };

} // namespace p2p
} // namespace fetch

#endif //FETCH_P2P_MUDDLE_HPP
