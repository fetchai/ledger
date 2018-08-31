
#include <network/p2pservice/p2p_service2.hpp>

#include "network/p2pservice/p2p_service2.hpp"

namespace fetch {
namespace p2p {


void P2PService2::Start(P2PService2::PortList const &ports,
                        P2PService2::PeerList const &initial_peer_list)
{
  // start the muddle server up
  muddle_.Start(ports, initial_peer_list);
}

void P2PService2::Stop()
{
  muddle_.Stop();
}

} // namespace p2p
} // namespace fetch
