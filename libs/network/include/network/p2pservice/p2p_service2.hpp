#pragma once

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
  using Muddle = muddle::Muddle;
  using PortList = Muddle::PortList;
  using PeerList = Muddle::PeerList;
  using RpcServer = muddle::rpc::Server;
  using ThreadPool = network::ThreadPool;

  // Construction / Destruction
  P2PService2(Muddle::CertificatePtr &&certificate, Muddle::NetworkManager const &nm)
    : muddle_(std::move(certificate), nm)
  {}

  void Start(PortList const &ports, PeerList const & initial_peer_list = PeerList{});
  void Stop();

private:

  Muddle muddle_;
  ThreadPool thread_pool_ = network::MakeThreadPool(1);
  RpcServer rpc_server_{muddle_.AsEndpoint(), 1};

  // address resolution service
  Resolver resolver_;
  ResolverProtocol resolver_proto_{resolver_};

};

} // namespace p2p
} // namespace fetch
