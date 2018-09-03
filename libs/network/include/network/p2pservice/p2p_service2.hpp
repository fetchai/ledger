#pragma once

#include "core/service_ids.hpp"
#include "network/details/thread_pool.hpp"
#include "network/muddle/muddle.hpp"
#include "network/peer.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_resolver_protocol.hpp"
#include "network/p2pservice/p2p_lane_management.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

namespace fetch {
namespace ledger { class LaneRemoteControl; }
namespace p2p {

class P2PService2
{
public:
  using NetworkManager = network::NetworkManager;
  using Muddle = muddle::Muddle;
  using PortList = Muddle::PortList;
  using PeerList = Muddle::PeerList;
  using RpcServer = muddle::rpc::Server;
  using ThreadPool = network::ThreadPool;
  using CertificatePtr = Muddle::CertificatePtr;
  using MuddleEndpoint = muddle::MuddleEndpoint;
  using Identity = crypto::Identity;
  using Peer   = network::Peer;
  using TrustInterface = P2PTrustInterface<Identity>;
  using LaneRemoteControl = ledger::LaneRemoteControl;
  using LaneRemoteControlPtr = std::shared_ptr<LaneRemoteControl>;
  using LaneRemoteControls = std::vector<LaneRemoteControlPtr>;

  enum
  {
    PROTOCOL_RESOLVER = 1
  };

  // Construction / Destruction
  P2PService2(Muddle &muddle, LaneManagement &lane_management);
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
  void WorkCycle();

private:

  Muddle  &muddle_;
  ThreadPool  thread_pool_ = network::MakeThreadPool(1);
  RpcServer rpc_server_{muddle_.AsEndpoint(), SERVICE_P2P, CHANNEL_RPC};

  LaneManagement &lane_management_;

  // address resolution service
  Resolver resolver_;
  ResolverProtocol resolver_proto_{resolver_};

  std::shared_ptr<TrustInterface> trustSystem;

  //std::set
  PeerList possibles_; // addresses we might use in the future.
  PeerList currently_trying_; // addresses we think we have inflight at the moment.
};

} // namespace p2p
} // namespace fetch
