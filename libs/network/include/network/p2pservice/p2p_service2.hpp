#pragma once

#include "network/details/thread_pool.hpp"
#include "network/muddle/muddle.hpp"
#include "network/peer.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_resolver_protocol.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"
#include "network/p2pservice/manifest.hpp"

namespace fetch {
namespace p2p {

class P2PService2
{
public:
  using Muddle = muddle::Muddle;
  using muddle_service_type  = std::shared_ptr<muddle::Muddle>;
  using PortList = Muddle::PortList;
  using PeerList = Muddle::PeerList;
  using RpcServer = muddle::rpc::Server;
  using ThreadPool = network::ThreadPool;
  using Identity    = crypto::Identity;
  using Peer   = network::Peer;
  using TrustInterface = P2PTrustInterface<Identity>;
  using Manifest = network::Manifest;

  // Construction / Destruction
  P2PService2(muddle_service_type muddle);
  ~P2PService2() = default;

  void Start(PeerList const & initial_peer_list = PeerList{});
  void Stop();

  void PeerIdentificationSucceeded(const Peer &peer, const Identity &identity);
  void PeerIdentificationFailed   (const Peer &peer);
  void PeerTrustEvent(const Identity &          identity
                      , P2PTrustFeedbackSubject subject
                      , P2PTrustFeedbackQuality quality);
  void WorkCycle();
private:

  muddle_service_type muddle_;
  ThreadPool thread_pool_ = network::MakeThreadPool(1);
  RpcServer rpc_server_{muddle_ -> AsEndpoint(), 1};

  // address resolution service
  Resolver resolver_;
  ResolverProtocol resolver_proto_{resolver_};

  std::shared_ptr<TrustInterface> trustSystem;
  std::map<Identity, Manifest> discoveredPeers;

  std::set

  PeerList possibles_; // addresses we might use in the future.
  PeerList currently_trying_; // addresses we think we have inflight at the moment.
};

} // namespace p2p
} // namespace fetch
