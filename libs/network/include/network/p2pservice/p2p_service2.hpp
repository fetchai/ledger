#pragma once

#include "core/service_ids.hpp"
#include "network/details/thread_pool.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_resolver_protocol.hpp"

namespace fetch {
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

  enum
  {
    PROTOCOL_RESOLVER = 1
  };

  // Construction / Destruction
  P2PService2(CertificatePtr &&certificate, NetworkManager const &nm);
  ~P2PService2() = default;

  void Start(PortList const &ports, PeerList const & initial_peer_list = PeerList{});
  void Stop();

  Identity const &identity() const { return muddle_.identity(); }
  MuddleEndpoint& AsEndpoint() { return muddle_.AsEndpoint(); }

private:

  Muddle      muddle_;
  ThreadPool  thread_pool_ = network::MakeThreadPool(1);
  RpcServer   rpc_server_{muddle_.AsEndpoint(), SERVICE_P2P, CHANNEL_RPC};

  // address resolution service
  Resolver resolver_;
  ResolverProtocol resolver_proto_{resolver_};

};

} // namespace p2p
} // namespace fetch
