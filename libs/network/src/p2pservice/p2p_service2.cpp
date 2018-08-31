#include <network/p2pservice/p2p_service2.hpp>
#include "network/p2pservice/p2p_service2.hpp"
#include "network/p2pservice/p2ptrust.hpp"

namespace fetch {
namespace p2p {

P2PService2::P2PService2(Muddle::CertificatePtr &&certificate, Muddle::NetworkManager const &nm)
  : muddle_(std::move(certificate), nm)
{
  // register the services with the rpc server
  rpc_server_.Add(1, &resolver_proto_);
  trustSystem = std::make_shared<P2PTrust<Identity>>();
}


void P2PService2::Start(P2PService2::PortList const &ports,
                        P2PService2::PeerList const &initial_peer_list)
{
  // start the muddle server up
  muddle_.Start(ports); //, initial_peer_list);

  for(auto &peer : initial_peer_list)
  {
    possibles_.push_back(peer);
  }
}

void P2PService2::Stop()
{
  muddle_.Stop();
}

void P2PService2::WorkCycle()
{
  // see how many peers we have.
  // not enough, schedule some connects.
  // too many? schedule some kickoffs.
}

void P2PService2::PeerIdentificationSucceeded(const P2PService2::Peer &peer, const P2PService2::Identity &identity)
{
}

void P2PService2::PeerIdentificationFailed   (const P2PService2::Peer &peer)
{
}

void P2PService2::PeerTrustEvent(const Identity &          identity
                                 , P2PTrustFeedbackSubject subject
                                 , P2PTrustFeedbackQuality quality)
{
}


} // namespace p2p
} // namespace fetch

