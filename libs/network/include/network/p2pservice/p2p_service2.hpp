#pragma once

#include "core/service_ids.hpp"
#include "network/details/thread_pool.hpp"
#include "network/muddle/muddle.hpp"
#include "network/peer.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_resolver_protocol.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"
#include "network/p2pservice/manifest.hpp"
#include "network/p2pservice/p2p_managed_local_service.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"

namespace fetch {
namespace p2p {

class P2PService2
{
public:
  using NetworkManager = network::NetworkManager;
  using Muddle = muddle::Muddle;
  using muddle_service_type  = muddle::Muddle&;
  using PortList = Muddle::PortList;
  using PeerList = Muddle::PeerList;
  using RpcServer = muddle::rpc::Server;
  using ThreadPool = network::ThreadPool;
  using CertificatePtr = Muddle::CertificatePtr;
  using MuddleEndpoint = muddle::MuddleEndpoint;
  using Identity = crypto::Identity;
  using Peer   = network::Peer;
  using TrustInterface = P2PTrustInterface<Identity>;
  using Manifest = network::Manifest;

  using Uri = network::Uri;
  using ServiceType = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;
  
  enum
  {
    PROTOCOL_RESOLVER = 1
  };
    static constexpr char const *LOGGING_NAME = "P2PService2";

  // Construction / Destruction
  P2PService2(Muddle &muddle);
  ~P2PService2() = default;

  void Start(PeerList const & initial_peer_list = PeerList{});
  void Stop();

  Identity const &identity() const { return muddle_ . identity(); }
  MuddleEndpoint& AsEndpoint() { return muddle_ . AsEndpoint(); }

  void PeerIdentificationSucceeded(const Peer &peer, const Identity &identity);
  void PeerIdentificationFailed   (const Peer &peer);
  void PeerTrustEvent(const Identity &          identity
                      , P2PTrustFeedbackSubject subject
                      , P2PTrustFeedbackQuality quality);

  void SetLocalManifest(const Manifest &manifest);

  void WorkCycle();
private:

  muddle_service_type muddle_;
  ThreadPool  thread_pool_ = network::MakeThreadPool(1);
  RpcServer rpc_server_{muddle_.AsEndpoint(), SERVICE_P2P, CHANNEL_RPC};

  // address resolution service
  Resolver resolver_;
  ResolverProtocol resolver_proto_{resolver_};

  std::shared_ptr<TrustInterface> trust_system;

  Manifest manifest_;
  std::map<Identity, Manifest> discovered_peers_;
  std::map<ServiceIdentifier, std::shared_ptr<P2PManagedLocalService>> local_services_;

  //std::set
  PeerList possibles_; // addresses we might use in the future.
  PeerList currently_trying_; // addresses we think we have inflight at the moment.
};

} // namespace p2p
} // namespace fetch
